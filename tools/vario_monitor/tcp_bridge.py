"""
TCP Bridge Server for streaming live NMEA/LK8EX1 sentences to remote clients.
Used by VarioAppli (Android emulator, smartphone over WiFi, or test harnesses).
"""

import socket
import threading
import select
from typing import Set, List, Callable, Optional


class TcpBridgeServer:
    def __init__(self, host: str = "0.0.0.0", port: int = 8888):
        self.host = host
        self.port = port
        self.is_running = False
        self._server_socket: Optional[socket.socket] = None
        self._clients: Set[socket.socket] = set()
        self._lock = threading.Lock()
        self._thread: Optional[threading.Thread] = None
        self.on_client_count_changed: Optional[Callable[[int], None]] = None

    def start(self) -> bool:
        """Starts the non-blocking TCP server."""
        if self.is_running:
            return True

        try:
            self._server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self._server_socket.bind((self.host, self.port))
            self._server_socket.listen(5)
            self._server_socket.setblocking(False)
            self.is_running = True
            
            self._thread = threading.Thread(target=self._run, daemon=True)
            self._thread.start()
            return True
        except Exception as e:
            print(f"[TCP Bridge] Failed to start server: {e}")
            self.stop()
            return False

    def _run(self):
        while self.is_running and self._server_socket:
            try:
                # Use select with timeout to allow clean shutdown
                r_list, _, _ = select.select([self._server_socket], [], [], 0.2)
                if self._server_socket in r_list:
                    client_sock, client_addr = self._server_socket.accept()
                    client_sock.setblocking(False)
                    with self._lock:
                        self._clients.add(client_sock)
                        count = len(self._clients)
                    print(f"[TCP Bridge] Client connected from {client_addr}. Total: {count}")
                    if self.on_client_count_changed:
                        self.on_client_count_changed(count)
            except Exception:
                pass

    def broadcast(self, raw_sentence: str):
        """Broadcasts a raw sentence to all connected TCP clients."""
        if not self.is_running or not self._clients:
            return

        payload = (raw_sentence.strip() + "\r\n").encode("ascii")
        to_remove = []

        with self._lock:
            for client in list(self._clients):
                try:
                    client.sendall(payload)
                except (socket.error, BrokenPipeError, ConnectionResetError):
                    to_remove.append(client)

            for dead_sock in to_remove:
                try:
                    dead_sock.close()
                except Exception:
                    pass
                self._clients.discard(dead_sock)

            if to_remove and self.on_client_count_changed:
                self.on_client_count_changed(len(self._clients))

    @property
    def client_count(self) -> int:
        with self._lock:
            return len(self._clients)

    def stop(self):
        """Stops the TCP server and closes all client sockets."""
        self.is_running = False
        with self._lock:
            for client in self._clients:
                try:
                    client.close()
                except Exception:
                    pass
            self._clients.clear()

        if self._server_socket:
            try:
                self._server_socket.close()
            except Exception:
                pass
            self._server_socket = None

        if self.on_client_count_changed:
            self.on_client_count_changed(0)
