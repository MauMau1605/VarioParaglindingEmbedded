"""
Thread-safe flight data logger for raw NMEA sentences and CSV exports.
"""

import os
import csv
import time
from datetime import datetime
from typing import Optional, Dict, Any


class FlightLogger:
    def __init__(self, base_dir: Optional[str] = None):
        if base_dir is None:
            base_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "logs")
        self.base_dir = base_dir
        self.is_recording = False
        self.file_handle = None
        self.csv_writer = None
        self.csv_handle = None
        self.current_filename = ""
        self.packet_count = 0
        self.byte_count = 0
        self.start_time = 0.0

    def start(self, filename_prefix: str = "flight_log", save_csv: bool = True) -> str:
        """Starts a recording session. Returns the created file path."""
        if self.is_recording:
            self.stop()

        os.makedirs(self.base_dir, exist_ok=True)
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.current_filename = os.path.join(self.base_dir, f"{filename_prefix}_{timestamp}.nmea")
        
        self.file_handle = open(self.current_filename, "w", encoding="ascii", buffering=1)
        
        if save_csv:
            csv_path = os.path.join(self.base_dir, f"{filename_prefix}_{timestamp}.csv")
            self.csv_handle = open(csv_path, "w", newline="", encoding="utf-8")
            self.csv_writer = csv.writer(self.csv_handle)
            self.csv_writer.writerow([
                "timestamp_iso",
                "elapsed_sec",
                "pressure_pa",
                "pressure_hpa",
                "altitude_baro_m",
                "vario_cm_s",
                "vario_m_s",
                "temperature_c",
                "raw_sentence"
            ])
        else:
            self.csv_handle = None
            self.csv_writer = None

        self.packet_count = 0
        self.byte_count = 0
        self.start_time = time.time()
        self.is_recording = True
        return self.current_filename

    def write_packet(self, raw_sentence: str, parsed: Optional[Dict[str, Any]] = None):
        """Write a packet to the active log file(s)."""
        if not self.is_recording or self.file_handle is None:
            return

        line = raw_sentence.strip() + "\r\n"
        self.file_handle.write(line)
        self.file_handle.flush()
        self.byte_count += len(line)
        self.packet_count += 1

        if self.csv_writer and parsed:
            now = time.time()
            iso_ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
            elapsed = round(now - self.start_time, 3)
            self.csv_writer.writerow([
                iso_ts,
                elapsed,
                parsed.get("pressure_pa", 0),
                round(parsed.get("pressure_hpa", 0.0), 2),
                round(parsed.get("computed_altitude_m", 0.0), 1),
                parsed.get("vario_cm_s", 0),
                round(parsed.get("vario_m_s", 0.0), 2),
                parsed.get("temperature_c", 0.0),
                parsed.get("raw", "")
            ])
            self.csv_handle.flush()

    def stop(self) -> Dict[str, Any]:
        """Stops the recording session and returns summary statistics."""
        if not self.is_recording:
            return {"packet_count": 0, "duration_s": 0.0, "filename": ""}

        duration = time.time() - self.start_time
        summary = {
            "filename": self.current_filename,
            "packet_count": self.packet_count,
            "byte_count": self.byte_count,
            "duration_s": round(duration, 1),
        }

        if self.file_handle:
            try:
                self.file_handle.close()
            except Exception:
                pass
            self.file_handle = None

        if self.csv_handle:
            try:
                self.csv_handle.close()
            except Exception:
                pass
            self.csv_handle = None
            self.csv_writer = None

        self.is_recording = False
        return summary
