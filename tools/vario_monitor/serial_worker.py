"""
Serial worker thread and flight simulator for VarioUSB monitor.
Handles serial communication, SAMD21 DTR/RTS signalling, and diagnostic logging.
"""

import time
import math
import queue
import threading
from typing import Optional, List, Dict, Any

try:
    import serial
    import serial.tools.list_ports
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

from .protocol import parse_lk8ex1, format_lk8ex1


def get_available_ports() -> List[str]:
    """Returns a list of available serial port names with USB VID:PID info."""
    if not HAS_SERIAL:
        return ["Simulateur Intégré (Thermique)"]

    ports = []
    for port_info in serial.tools.list_ports.comports():
        vid_pid = ""
        if port_info.vid is not None and port_info.pid is not None:
            vid_pid = f" [VID:{port_info.vid:04X} PID:{port_info.pid:04X}]"
            # Note: Seeed XIAO SAMD21 default VID is 0x2886, PID is 0x802F (app) or 0x002F (bootloader)
            if port_info.vid == 0x2886:
                vid_pid += " (Seeed XIAO SAMD21)"

        desc = port_info.description or "Périphérique série"
        ports.append(f"{port_info.device} ({desc}{vid_pid})")

    ports.append("Simulateur Intégré (Thermique)")
    return ports


class SerialWorker:
    def __init__(self, data_queue: queue.Queue):
        self.data_queue = data_queue
        self.is_running = False
        self._thread: Optional[threading.Thread] = None
        self._serial: Optional[Any] = None
        self.is_simulated = False
        self.port_name = ""
        self.baudrate = 115200

        # Diagnostics & Metrics
        self.debug_mode = True
        self.packets_received = 0
        self.bytes_received = 0
        self.checksum_errors = 0
        self.last_rate_time = time.time()
        self.rate_packet_count = 0
        self.current_fps = 0.0

    def log_debug(self, message: str):
        """Sends a debug log message to the UI queue and terminal."""
        print(f"[SERIAL DEBUG] {message}")
        self.data_queue.put({"type": "debug", "message": message})

    def start(self, port: str, baudrate: int = 115200, debug: bool = True) -> bool:
        """Starts the worker thread."""
        if self.is_running:
            self.stop()

        self.port_name = port
        self.baudrate = baudrate
        self.debug_mode = debug
        self.is_simulated = "Simulateur" in port
        self.packets_received = 0
        self.bytes_received = 0
        self.checksum_errors = 0
        self.rate_packet_count = 0
        self.last_rate_time = time.time()
        self.current_fps = 0.0

        if not self.is_simulated:
            if not HAS_SERIAL:
                err_msg = "Module 'pyserial' non installé."
                self.data_queue.put({"type": "error", "message": err_msg})
                return False

            clean_port = port.split()[0]
            self.log_debug(f"Tentative de connexion sur {clean_port} à {baudrate} bauds...")

            try:
                # Open serial port
                self._serial = serial.Serial(
                    port=clean_port,
                    baudrate=baudrate,
                    timeout=0.1,
                    write_timeout=1.0,
                    xonxoff=False,
                    rtscts=False,
                    dsrdtr=False
                )

                # IMPORTANT: ATSAMD21 Native USB CDC requires DTR=True
                # In Arduino SAMD21, `if (Serial)` or `while (!Serial)` checks if DTR line is asserted!
                self._serial.dtr = True
                self._serial.rts = True
                time.sleep(0.05)  # brief settle time

                # Clear old junk in buffer
                self._serial.reset_input_buffer()

                self.log_debug(
                    f"Port {clean_port} ouvert. Signaux DTR=True & RTS=True activés "
                    f"(requis pour débloquer l'émission USB CDC sur Seeed XIAO SAMD21)."
                )

            except Exception as e:
                err_msg = f"Impossible d'ouvrir le port {clean_port}: {e}"
                print(f"[SERIAL ERROR] {err_msg}")
                self.data_queue.put({"type": "error", "message": err_msg})
                return False
        else:
            self.log_debug("Démarrage en mode SIMULATEUR de vol (génération synthétique 10 Hz).")

        self.is_running = True
        self._thread = threading.Thread(
            target=self._run_simulation if self.is_simulated else self._run_serial,
            daemon=True
        )
        self._thread.start()
        self.data_queue.put({"type": "connected", "port": port, "is_simulated": self.is_simulated})
        return True

    def _run_serial(self):
        """Worker loop reading serial data with comprehensive diagnostics."""
        line_buffer = bytearray()
        last_data_time = time.time()
        last_silence_report = 0.0

        self.log_debug("Boucle de lecture série démarrée. En attente du premier octet...")

        while self.is_running and self._serial and self._serial.is_open:
            try:
                waiting = self._serial.in_waiting
                if waiting > 0:
                    raw_bytes = self._serial.read(min(waiting, 128))
                else:
                    raw_bytes = self._serial.read(32)

                if not raw_bytes:
                    now = time.time()
                    elapsed_silence = now - last_data_time

                    # Periodic silence diagnostic (after 3s, then every 6s)
                    if elapsed_silence >= 3.0 and (now - last_silence_report) >= 5.0:
                        last_silence_report = now
                        msg = (
                            f"Aucun octet reçu depuis {int(elapsed_silence)}s. "
                            f"Port {self._serial.port} ouvert, DTR={self._serial.dtr}. "
                            f"\n -> Vérifiez si la LED orange de la XIAO clignote rapidement à 5 Hz "
                            f"(si oui: échec initialisation capteur BMP390 sur le bus I2C)."
                        )
                        self.log_debug(msg)
                    continue

                # Data received!
                last_data_time = time.time()
                self.bytes_received += len(raw_bytes)

                # Show first reception debug
                if self.bytes_received <= len(raw_bytes) + 10:
                    preview = repr(raw_bytes[:40])
                    self.log_debug(f"Premiers octets reçus ({len(raw_bytes)} octets) : {preview}")

                for b in raw_bytes:
                    if b == ord('\n'):
                        raw_line = line_buffer.decode("ascii", errors="replace").strip()
                        line_buffer.clear()
                        if raw_line:
                            self._handle_sentence(raw_line)
                    elif b != ord('\r'):
                        line_buffer.append(b)
                        if len(line_buffer) > 256:
                            # Buffer overflow without newline: dump raw contents for diagnosis
                            dump = line_buffer.decode("ascii", errors="replace")
                            self.log_debug(f"Trame trop longue (>256 octets sans saut de ligne) : {dump[:80]}...")
                            line_buffer.clear()

            except serial.SerialException as e:
                err = f"Erreur de communication série: {e}"
                print(f"[SERIAL ERROR] {err}")
                self.data_queue.put({"type": "error", "message": err})
                break
            except Exception as e:
                err = f"Erreur inattendue dans le thread série: {e}"
                print(f"[SERIAL ERROR] {err}")
                self.data_queue.put({"type": "error", "message": err})
                break

        self.log_debug("Arrêt de la boucle de lecture série.")
        self.stop()

    def _run_simulation(self):
        """Flight simulation generating thermal and sink patterns at 10 Hz."""
        sim_time = 0.0
        base_pressure = 95000.0  # Approx 540 m altitude
        base_temp = 22.0

        self.log_debug("Générateur thermique actif (10 trames/sec).")

        while self.is_running:
            time.sleep(0.1)  # 10 Hz output
            sim_time += 0.1

            cycle = sim_time % 45.0
            if cycle < 20.0:
                vz_m_s = 2.4 + 1.2 * math.sin(sim_time * 0.8) + 0.2 * math.sin(sim_time * 2.5)
            elif cycle < 30.0:
                vz_m_s = -0.3 + 0.2 * math.sin(sim_time * 1.2)
            else:
                vz_m_s = -1.8 - 0.7 * math.sin(sim_time * 0.6)

            base_pressure -= vz_m_s * 0.1 * 11.8
            vario_cm_s = int(round(vz_m_s * 100))
            current_pa = int(round(base_pressure))
            temp_c = int(round(base_temp - (sim_time * 0.01)))

            sim_sentence = format_lk8ex1(
                pressure_pa=current_pa,
                vario_cm_s=vario_cm_s,
                temperature_c=temp_c
            )
            self._handle_sentence(sim_sentence.strip())

    def _handle_sentence(self, raw_sentence: str):
        """Processes and enqueues a received sentence."""
        parsed = parse_lk8ex1(raw_sentence)
        self.packets_received += 1
        self.rate_packet_count += 1

        now = time.time()
        elapsed = now - self.last_rate_time
        if elapsed >= 1.0:
            self.current_fps = round(self.rate_packet_count / elapsed, 1)
            self.rate_packet_count = 0
            self.last_rate_time = now

        if parsed:
            if not parsed["valid_checksum"]:
                self.checksum_errors += 1
                self.log_debug(f"Checksum invalide sur trame : {raw_sentence}")
        else:
            self.checksum_errors += 1
            # Could be an unexpected sentence or firmware debug print
            self.log_debug(f"Trame non reconnue comme LK8EX1 : {raw_sentence}")

        self.data_queue.put({
            "type": "packet",
            "raw": raw_sentence,
            "parsed": parsed,
            "fps": self.current_fps,
            "total_packets": self.packets_received,
            "checksum_errors": self.checksum_errors
        })

    def stop(self):
        """Stops reading and closes the connection."""
        if not self.is_running:
            return

        self.is_running = False
        if self._serial:
            try:
                self._serial.close()
            except Exception:
                pass
            self._serial = None

        self.data_queue.put({"type": "disconnected"})
