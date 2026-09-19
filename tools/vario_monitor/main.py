"""
VarioUSB Telemetry Monitor & Flight Recorder GUI
GUI application for testing Seeed XIAO SAMD21 + BMP390 LK8EX1 telemetry,
recording flight data, and streaming to VarioAppli via TCP bridge.
"""

import sys
import os
import queue
import time
import tkinter as tk
from tkinter import ttk, messagebox, filedialog

# Support local package imports whether executed from root or inside tools/vario_monitor/
if __name__ == "__main__" and __package__ is None:
    sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    from vario_monitor.protocol import parse_lk8ex1
    from vario_monitor.serial_worker import SerialWorker, get_available_ports
    from vario_monitor.tcp_bridge import TcpBridgeServer
    from vario_monitor.flight_logger import FlightLogger
else:
    from .protocol import parse_lk8ex1
    from .serial_worker import SerialWorker, get_available_ports
    from .tcp_bridge import TcpBridgeServer
    from .flight_logger import FlightLogger


class VarioMonitorApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("VarioUSB — Moniteur Télémétrie & Enregistreur de Vol")
        self.geometry("980x780")
        self.minsize(820, 640)

        # Style & Colors (Dark Cockpit Theme)
        self.configure(bg="#0D1117")
        self.COLOR_BG = "#0D1117"
        self.COLOR_CARD = "#161B22"
        self.COLOR_CARD_BORDER = "#30363D"
        self.COLOR_TEXT_MAIN = "#F0F6FC"
        self.COLOR_TEXT_MUTED = "#8B949E"
        self.COLOR_ACCENT_GREEN = "#2EA043"
        self.COLOR_ACCENT_RED = "#DA3633"
        self.COLOR_ACCENT_BLUE = "#58A6FF"
        self.COLOR_ACCENT_CYAN = "#39C5BB"
        self.COLOR_ACCENT_ORANGE = "#D29922"

        # Worker & Engine components
        self.data_queue = queue.Queue()
        self.serial_worker = SerialWorker(self.data_queue)
        self.flight_logger = FlightLogger()
        self.tcp_bridge = TcpBridgeServer(host="0.0.0.0", port=8888)

        # State tracking
        self.auto_scroll = tk.BooleanVar(value=True)
        self.record_csv = tk.BooleanVar(value=True)
        self.tcp_bridge_enabled = tk.BooleanVar(value=False)
        self.debug_logs_enabled = tk.BooleanVar(value=True)
        self.initial_altitude_m = None

        self._configure_styles()
        self._build_ui()

        # Wire up bridge callbacks
        self.tcp_bridge.on_client_count_changed = self._on_bridge_client_count_changed

        # Polling queue
        self.after(50, self._process_queue)
        self.protocol("WM_DELETE_WINDOW", self._on_closing)

    def _configure_styles(self):
        style = ttk.Style(self)
        style.theme_use("clam")
        
        style.configure("TFrame", background=self.COLOR_BG)
        style.configure("Card.TFrame", background=self.COLOR_CARD, relief="flat")
        style.configure("TLabel", background=self.COLOR_BG, foreground=self.COLOR_TEXT_MAIN, font=("Segoe UI", 10))
        style.configure("Muted.TLabel", background=self.COLOR_CARD, foreground=self.COLOR_TEXT_MUTED, font=("Segoe UI", 9))
        style.configure("CardTitle.TLabel", background=self.COLOR_CARD, foreground=self.COLOR_TEXT_MUTED, font=("Segoe UI", 10, "bold"))
        style.configure("CardVal.TLabel", background=self.COLOR_CARD, foreground=self.COLOR_TEXT_MAIN, font=("Segoe UI", 22, "bold"))
        style.configure("TButton", font=("Segoe UI", 9, "bold"), padding=6)
        style.configure("Action.TButton", font=("Segoe UI", 10, "bold"), padding=8)
        style.configure("TCheckbutton", background=self.COLOR_CARD, foreground=self.COLOR_TEXT_MAIN, font=("Segoe UI", 9))

    def _build_ui(self):
        # ── 1. Top Bar: Connection & Port Controls ─────────────────────────
        top_bar = tk.Frame(self, bg=self.COLOR_CARD, highlightbackground=self.COLOR_CARD_BORDER, highlightthickness=1)
        top_bar.pack(fill="x", padx=16, pady=(12, 8))

        lbl_port = tk.Label(top_bar, text="Port Série:", bg=self.COLOR_CARD, fg=self.COLOR_TEXT_MAIN, font=("Segoe UI", 10, "bold"))
        lbl_port.pack(side="left", padx=(12, 6), pady=10)

        self.cb_ports = ttk.Combobox(top_bar, width=38, state="readonly")
        self.cb_ports.pack(side="left", padx=4, pady=10)
        self._refresh_ports(initial=True)

        btn_refresh = tk.Button(top_bar, text="🔄", command=lambda: self._refresh_ports(initial=False), bg="#21262D", fg=self.COLOR_TEXT_MAIN, relief="flat", padx=6, pady=2)
        btn_refresh.pack(side="left", padx=4, pady=10)

        lbl_baud = tk.Label(top_bar, text="Baud:", bg=self.COLOR_CARD, fg=self.COLOR_TEXT_MUTED, font=("Segoe UI", 9))
        lbl_baud.pack(side="left", padx=(12, 4), pady=10)

        self.cb_baud = ttk.Combobox(top_bar, values=["115200", "57600", "9600"], width=8, state="readonly")
        self.cb_baud.set("115200")
        self.cb_baud.pack(side="left", padx=4, pady=10)

        self.btn_connect = tk.Button(
            top_bar, text="Connecter", command=self._toggle_connection,
            bg=self.COLOR_ACCENT_GREEN, fg="#FFFFFF", font=("Segoe UI", 10, "bold"),
            relief="flat", padx=16, pady=4, cursor="hand2"
        )
        self.btn_connect.pack(side="left", padx=(16, 12), pady=10)

        self.lbl_status = tk.Label(top_bar, text="⚪ Déconnecté", bg=self.COLOR_CARD, fg=self.COLOR_TEXT_MUTED, font=("Segoe UI", 10))
        self.lbl_status.pack(side="right", padx=16, pady=10)

        # ── 2. Telemetry Cards (Vz, Pression, Altitude, Temp, Stats) ────────
        cards_frame = tk.Frame(self, bg=self.COLOR_BG)
        cards_frame.pack(fill="x", padx=16, pady=4)
        for i in range(5):
            cards_frame.columnconfigure(i, weight=1)

        # Card 1: Vario Vz
        self.card_vz = self._create_card(cards_frame, 0, "VARIO (Vz)", "0.00 m/s", "0 cm/s")
        # Card 2: Pression
        self.card_press = self._create_card(cards_frame, 1, "PRESSION", "--- hPa", "--- Pa")
        # Card 3: Altitude
        self.card_alt = self._create_card(cards_frame, 2, "ALTITUDE BARO", "--- m", "Ref: QNH 1013.25")
        # Card 4: Température
        self.card_temp = self._create_card(cards_frame, 3, "TEMPÉRATURE", "--- °C", "Capteur BMP390")
        # Card 5: Fréquence & Intégrité
        self.card_stats = self._create_card(cards_frame, 4, "FLUX / CHK", "0.0 Hz", "0 trames (100% OK)")

        # ── 3. Middle Bar: Enregistreur & Pont TCP pour VarioAppli ─────────
        mid_frame = tk.Frame(self, bg=self.COLOR_CARD, highlightbackground=self.COLOR_CARD_BORDER, highlightthickness=1)
        mid_frame.pack(fill="x", padx=16, pady=8)

        # Logger sub-panel (Left)
        rec_frame = tk.Frame(mid_frame, bg=self.COLOR_CARD)
        rec_frame.pack(side="left", fill="both", expand=True, padx=14, pady=10)

        lbl_rec_title = tk.Label(rec_frame, text="📁 ENREGISTREMENT DE VOL", bg=self.COLOR_CARD, fg=self.COLOR_TEXT_MUTED, font=("Segoe UI", 9, "bold"))
        lbl_rec_title.pack(anchor="w")

        rec_ctrl = tk.Frame(rec_frame, bg=self.COLOR_CARD)
        rec_ctrl.pack(anchor="w", pady=(6, 2))

        self.btn_record = tk.Button(
            rec_ctrl, text="🔴 Démarrer l'enregistrement", command=self._toggle_recording,
            bg=self.COLOR_ACCENT_RED, fg="#FFFFFF", font=("Segoe UI", 9, "bold"),
            relief="flat", padx=10, pady=3, cursor="hand2"
        )
        self.btn_record.pack(side="left", padx=(0, 10))

        chk_csv = tk.Checkbutton(
            rec_ctrl, text="Exporter aussi en .CSV", variable=self.record_csv,
            bg=self.COLOR_CARD, fg=self.COLOR_TEXT_MAIN, selectcolor=self.COLOR_CARD,
            activebackground=self.COLOR_CARD, activeforeground=self.COLOR_TEXT_MAIN
        )
        chk_csv.pack(side="left")

        btn_open_folder = tk.Button(
            rec_ctrl, text="Ouvrir dossier logs", command=self._open_logs_folder,
            bg="#21262D", fg=self.COLOR_TEXT_MAIN, relief="flat", padx=8, pady=2
        )
        btn_open_folder.pack(side="left", padx=10)

        self.lbl_record_status = tk.Label(rec_frame, text="Prêt (enregistrements sauvés dans tools/vario_monitor/logs/)", bg=self.COLOR_CARD, fg=self.COLOR_TEXT_MUTED, font=("Segoe UI", 8))
        self.lbl_record_status.pack(anchor="w", pady=(4, 0))

        # TCP Bridge sub-panel (Right - Link to VarioAppli)
        bridge_frame = tk.Frame(mid_frame, bg=self.COLOR_CARD)
        bridge_frame.pack(side="right", fill="both", expand=True, padx=14, pady=10)

        lbl_bridge_title = tk.Label(bridge_frame, text="🌐 LIEN VERS VARIOAPPLI (PONT TCP)", bg=self.COLOR_CARD, fg=self.COLOR_TEXT_MUTED, font=("Segoe UI", 9, "bold"))
        lbl_bridge_title.pack(anchor="w")

        bridge_ctrl = tk.Frame(bridge_frame, bg=self.COLOR_CARD)
        bridge_ctrl.pack(anchor="w", pady=(6, 2))

        chk_bridge = tk.Checkbutton(
            bridge_ctrl, text="Activer Serveur TCP (Port 8888)", variable=self.tcp_bridge_enabled,
            command=self._toggle_tcp_bridge,
            bg=self.COLOR_CARD, fg=self.COLOR_TEXT_MAIN, selectcolor=self.COLOR_CARD,
            activebackground=self.COLOR_CARD, activeforeground=self.COLOR_TEXT_MAIN,
            font=("Segoe UI", 9, "bold")
        )
        chk_bridge.pack(side="left", padx=(0, 10))

        btn_bridge_help = tk.Button(
            bridge_ctrl, text="ℹ️ Comment connecter VarioAppli", command=self._show_varioappli_instructions,
            bg="#21262D", fg=self.COLOR_ACCENT_BLUE, relief="flat", padx=8, pady=2
        )
        btn_bridge_help.pack(side="left")

        self.lbl_bridge_status = tk.Label(
            bridge_frame, text="Serveur TCP inactif. Cochez pour diffuser les trames en direct vers VarioAppli.",
            bg=self.COLOR_CARD, fg=self.COLOR_TEXT_MUTED, font=("Segoe UI", 8)
        )
        self.lbl_bridge_status.pack(anchor="w", pady=(4, 0))

        # ── 4. Bottom Panel: Raw Frames Console & Debug Logs ───────────────
        console_panel = tk.Frame(self, bg=self.COLOR_CARD, highlightbackground=self.COLOR_CARD_BORDER, highlightthickness=1)
        console_panel.pack(fill="both", expand=True, padx=16, pady=(4, 12))

        header_console = tk.Frame(console_panel, bg=self.COLOR_CARD)
        header_console.pack(fill="x", padx=12, pady=(8, 4))

        lbl_console = tk.Label(header_console, text="📺 CONSOLE DES TRAMES & LOGS DE DÉBOGAGE", bg=self.COLOR_CARD, fg=self.COLOR_TEXT_MAIN, font=("Segoe UI", 10, "bold"))
        lbl_console.pack(side="left")

        chk_debug = tk.Checkbutton(
            header_console, text="Logs de débogage", variable=self.debug_logs_enabled,
            bg=self.COLOR_CARD, fg=self.COLOR_ACCENT_ORANGE, selectcolor=self.COLOR_CARD,
            activebackground=self.COLOR_CARD, activeforeground=self.COLOR_ACCENT_ORANGE
        )
        chk_debug.pack(side="right", padx=(8, 0))

        chk_scroll = tk.Checkbutton(
            header_console, text="Défilement auto", variable=self.auto_scroll,
            bg=self.COLOR_CARD, fg=self.COLOR_TEXT_MAIN, selectcolor=self.COLOR_CARD,
            activebackground=self.COLOR_CARD, activeforeground=self.COLOR_TEXT_MAIN
        )
        chk_scroll.pack(side="right", padx=(8, 0))

        btn_clear = tk.Button(
            header_console, text="Effacer", command=self._clear_console,
            bg="#21262D", fg=self.COLOR_TEXT_MAIN, relief="flat", padx=8, pady=2
        )
        btn_clear.pack(side="right")

        # Monospaced text widget with scrollbar
        text_container = tk.Frame(console_panel, bg="#0A0C10")
        text_container.pack(fill="both", expand=True, padx=10, pady=(2, 10))

        self.txt_console = tk.Text(
            text_container, bg="#0A0C10", fg="#C9D1D9", insertbackground="#FFFFFF",
            font=("Consolas", 10), wrap="none", relief="flat", borderwidth=0
        )
        scroll_y = ttk.Scrollbar(text_container, orient="vertical", command=self.txt_console.yview)
        self.txt_console.configure(yscrollcommand=scroll_y.set)

        scroll_y.pack(side="right", fill="y")
        self.txt_console.pack(side="left", fill="both", expand=True)

        # Syntax tags for console
        self.txt_console.tag_config("valid", foreground="#7EE787")
        self.txt_console.tag_config("error", foreground="#FF7B72")
        self.txt_console.tag_config("info", foreground="#79C0FF")
        self.txt_console.tag_config("debug", foreground="#D29922")
        self.txt_console.tag_config("timestamp", foreground="#8B949E")

        # Initial greeting line
        self._append_console("Système initialisé. Cliquez sur 'Connecter' pour démarrer la lecture du port COM sélectionné.", "info")

    def _create_card(self, parent, col, title, initial_val, initial_sub):
        card = tk.Frame(parent, bg=self.COLOR_CARD, highlightbackground=self.COLOR_CARD_BORDER, highlightthickness=1)
        card.grid(row=0, column=col, padx=4, sticky="nsew")

        lbl_title = tk.Label(card, text=title, bg=self.COLOR_CARD, fg=self.COLOR_TEXT_MUTED, font=("Segoe UI", 9, "bold"))
        lbl_title.pack(anchor="w", padx=10, pady=(8, 2))

        lbl_val = tk.Label(card, text=initial_val, bg=self.COLOR_CARD, fg=self.COLOR_TEXT_MAIN, font=("Segoe UI", 18, "bold"))
        lbl_val.pack(anchor="w", padx=10, pady=(0, 2))

        lbl_sub = tk.Label(card, text=initial_sub, bg=self.COLOR_CARD, fg=self.COLOR_TEXT_MUTED, font=("Segoe UI", 8))
        lbl_sub.pack(anchor="w", padx=10, pady=(0, 8))

        return {"frame": card, "title": lbl_title, "val": lbl_val, "sub": lbl_sub}

    def _refresh_ports(self, initial: bool = False):
        ports = get_available_ports()
        self.cb_ports["values"] = ports
        if ports:
            self.cb_ports.current(0)
        
        if not initial:
            self._append_console(f"Recherche des ports : {len(ports)} option(s) détectée(s).", "debug")
            for p in ports:
                self._append_console(f"  • {p}", "debug")

    def _toggle_connection(self):
        if self.serial_worker.is_running:
            self.serial_worker.stop()
            self.btn_connect.configure(text="Connecter", bg=self.COLOR_ACCENT_GREEN)
            self.lbl_status.configure(text="⚪ Déconnecté", fg=self.COLOR_TEXT_MUTED)
            self._append_console("Déconnexion demandée par l'utilisateur.", "info")
        else:
            selected_port = self.cb_ports.get()
            if not selected_port:
                messagebox.showwarning("Attention", "Veuillez sélectionner un port ou le simulateur.")
                return

            baud = int(self.cb_baud.get() or 115200)
            self._append_console(f"Lancement de la connexion vers {selected_port} à {baud} bauds...", "info")
            if self.serial_worker.start(selected_port, baud, debug=self.debug_logs_enabled.get()):
                self.btn_connect.configure(text="Déconnecter", bg=self.COLOR_ACCENT_RED)
                mode_desc = "Simulateur Vol" if "Simulateur" in selected_port else f"Série {selected_port.split()[0]}"
                self.lbl_status.configure(text=f"🟢 Connecté ({mode_desc})", fg=self.COLOR_ACCENT_GREEN)

    def _toggle_recording(self):
        if self.flight_logger.is_recording:
            summary = self.flight_logger.stop()
            self.btn_record.configure(text="🔴 Démarrer l'enregistrement", bg=self.COLOR_ACCENT_RED)
            self.lbl_record_status.configure(
                text=f"Enregistrement terminé: {summary['packet_count']} trames ({summary['duration_s']}s) dans {os.path.basename(summary['filename'])}",
                fg=self.COLOR_ACCENT_GREEN
            )
            self._append_console(f"Enregistrement clos: {summary['filename']}", "info")
        else:
            filename = self.flight_logger.start(save_csv=self.record_csv.get())
            self.btn_record.configure(text="⏹️ Arrêter l'enregistrement", bg=self.COLOR_ACCENT_ORANGE)
            self.lbl_record_status.configure(
                text=f"Enregistrement actif vers: {os.path.basename(filename)}",
                fg=self.COLOR_ACCENT_ORANGE
            )
            self._append_console(f"Début enregistrement: {filename}", "info")

    def _open_logs_folder(self):
        folder = self.flight_logger.base_dir
        os.makedirs(folder, exist_ok=True)
        if sys.platform == "win32":
            os.startfile(folder)
        else:
            import subprocess
            subprocess.Popen(["xdg-open", folder])

    def _toggle_tcp_bridge(self):
        if self.tcp_bridge_enabled.get():
            if self.tcp_bridge.start():
                self.lbl_bridge_status.configure(
                    text=f"🟢 Pont TCP actif sur 0.0.0.0:8888 ({self.tcp_bridge.client_count} client(s))",
                    fg=self.COLOR_ACCENT_GREEN
                )
                self._append_console("Serveur Bridge TCP démarré sur port 8888. Prêt pour VarioAppli.", "info")
            else:
                self.tcp_bridge_enabled.set(False)
                messagebox.showerror("Erreur", "Impossible d'ouvrir le port TCP 8888.")
        else:
            self.tcp_bridge.stop()
            self.lbl_bridge_status.configure(
                text="Serveur TCP inactif. Cochez pour diffuser les trames en direct vers VarioAppli.",
                fg=self.COLOR_TEXT_MUTED
            )
            self._append_console("Serveur Bridge TCP arrêté.", "info")

    def _on_bridge_client_count_changed(self, count: int):
        self.after(0, lambda: self.lbl_bridge_status.configure(
            text=f"🟢 Pont TCP actif sur 0.0.0.0:8888 ({count} client(s) connecté(s))",
            fg=self.COLOR_ACCENT_GREEN if count > 0 else self.COLOR_ACCENT_CYAN
        ))

    def _show_varioappli_instructions(self):
        msg = (
            "Comment relier les données à VarioAppli (situé dans ../VarioAppli) :\n\n"
            "1. Mode direct Smartphone (USB OTG) :\n"
            "   Branchez directement le dongle Seeed XIAO SAMD21 sur le smartphone via un câble USB OTG. "
            "L'application Android native utilise la librairie usb-serial-for-android et lit les trames LK8EX1.\n\n"
            "2. Mode Débogage / Émulateur Android (via le Pont TCP) :\n"
            "   - Activez le pont TCP (Port 8888) dans ce moniteur.\n"
            "   - Si vous utilisez l'émulateur Android Studio : connectez un socket TCP client vers 10.0.2.2:8888.\n"
            "   - Si vous utilisez un smartphone sur le même WiFi : connectez un socket client vers l'IP locale de votre PC:8888.\n"
            "   - Toutes les trames reçues du capteur ou générées par le simulateur y sont injectées en direct !\n\n"
            "3. Mode Rejeu de vol (Replay / Test vectors) :\n"
            "   Les fichiers .nmea et .csv enregistrés peuvent être copiés dans VarioAppli/app/src/test/ "
            "pour alimenter les tests unitaires (ex: Lk8ex1ParserTest) ou tester la synthèse audio."
        )
        messagebox.showinfo("Liaison avec VarioAppli", msg)

    def _process_queue(self):
        """Read pending items from the serial worker thread."""
        limit = 30  # avoid starving GUI if lots of messages
        while limit > 0 and not self.data_queue.empty():
            limit -= 1
            try:
                msg = self.data_queue.get_nowait()
            except queue.Empty:
                break

            msg_type = msg.get("type")
            if msg_type == "packet":
                self._handle_packet(msg)
            elif msg_type == "debug":
                if self.debug_logs_enabled.get():
                    self._append_console(f"[DEBUG] {msg.get('message')}", "debug")
            elif msg_type == "connected":
                port = msg.get("port")
                self._append_console(f"Connexion établie avec succès sur {port}.", "info")
            elif msg_type == "error":
                self._append_console(f"ERREUR: {msg.get('message')}", "error")
                self.lbl_status.configure(text="⚠️ Erreur port", fg=self.COLOR_ACCENT_RED)
            elif msg_type == "disconnected":
                self.btn_connect.configure(text="Connecter", bg=self.COLOR_ACCENT_GREEN)
                self.lbl_status.configure(text="⚪ Déconnecté", fg=self.COLOR_TEXT_MUTED)
                self._append_console("Port série fermé.", "info")

        self.after(40, self._process_queue)

    def _handle_packet(self, msg: dict):
        raw = msg.get("raw", "")
        parsed = msg.get("parsed")
        fps = msg.get("fps", 0.0)
        total = msg.get("total_packets", 0)
        errors = msg.get("checksum_errors", 0)

        # 1. Forward to active logger
        if self.flight_logger.is_recording:
            self.flight_logger.write_packet(raw, parsed)
            self.lbl_record_status.configure(
                text=f"Enregistrement: {self.flight_logger.packet_count} trames ({round(self.flight_logger.byte_count / 1024, 1)} Ko)",
                fg=self.COLOR_ACCENT_ORANGE
            )

        # 2. Forward to TCP bridge for VarioAppli / remote clients
        if self.tcp_bridge.is_running:
            self.tcp_bridge.broadcast(raw)

        # 3. Update Console
        if parsed and parsed.get("valid_checksum"):
            self._append_console(raw, "valid")
        else:
            self._append_console(raw, "error")

        # 4. Update Telemetry Cards
        if parsed:
            vz = parsed["vario_m_s"]
            vz_cm = parsed["vario_cm_s"]
            vz_str = f"{vz:+.2f} m/s"
            
            # Dynamic Vz color
            if vz > 0.15:
                vz_color = self.COLOR_ACCENT_GREEN
            elif vz < -0.30:
                vz_color = self.COLOR_ACCENT_RED
            else:
                vz_color = self.COLOR_TEXT_MAIN

            self.card_vz["val"].configure(text=vz_str, fg=vz_color)
            self.card_vz["sub"].configure(text=f"{vz_cm:+} cm/s")

            # Pressure
            hpa = parsed["pressure_hpa"]
            pa = parsed["pressure_pa"]
            self.card_press["val"].configure(text=f"{hpa:.2f} hPa")
            self.card_press["sub"].configure(text=f"{pa} Pa")

            # Altitude
            alt_m = parsed["computed_altitude_m"]
            if self.initial_altitude_m is None:
                self.initial_altitude_m = alt_m
            rel_alt = alt_m - self.initial_altitude_m
            self.card_alt["val"].configure(text=f"{alt_m:.1f} m")
            self.card_alt["sub"].configure(text=f"Gain relatif: {rel_alt:+.1f} m")

            # Temperature
            temp = parsed["temperature_c"]
            self.card_temp["val"].configure(text=f"{temp:.1f} °C")

            # Flow & Checksum stats
            pct_ok = round(100.0 * (total - errors) / max(1, total), 1)
            self.card_stats["val"].configure(text=f"{fps:.1f} Hz")
            self.card_stats["sub"].configure(text=f"{total} trames ({pct_ok}% OK)")

    def _append_console(self, text: str, tag: str):
        now_str = time.strftime("%H:%M:%S")
        self.txt_console.insert(tk.END, f"[{now_str}] ", "timestamp")
        self.txt_console.insert(tk.END, f"{text}\n", tag)
        
        # Limit buffer length to prevent memory leaks during long flights
        lines = int(self.txt_console.index('end-1c').split('.')[0])
        if lines > 1500:
            self.txt_console.delete("1.0", "200.0")

        if self.auto_scroll.get():
            self.txt_console.see(tk.END)

    def _clear_console(self):
        self.txt_console.delete("1.0", tk.END)

    def _on_closing(self):
        if self.flight_logger.is_recording:
            self.flight_logger.stop()
        if self.serial_worker.is_running:
            self.serial_worker.stop()
        if self.tcp_bridge.is_running:
            self.tcp_bridge.stop()
        self.destroy()


def main():
    app = VarioMonitorApp()
    app.mainloop()


if __name__ == "__main__":
    main()
