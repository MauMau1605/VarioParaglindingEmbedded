#!/usr/bin/env python3
"""
Pytest integration for VarioUSB offline unit test suites.
Can be run via:
    pytest tests/
or
    python -m pytest tests/
"""

import sys
import os

# Ensure tests directory is on Python path
sys.path.insert(0, os.path.dirname(__file__))

from run_tests import (
    test_baro_math as run_baro_math,
    test_vario_dt as run_vario_dt,
    test_ubx_parser as run_ubx_parser,
    test_lk8ex1_formatter as run_lk8ex1_formatter,
    test_vario_engine as run_vario_engine,
    test_gnss_processor as run_gnss_processor,
    test_battery_monitor as run_battery_monitor,
    test_buzzer as run_buzzer,
    test_bmp390_compensation as run_bmp390_compensation
)

def test_baro_math():
    run_baro_math()

def test_vario_dt():
    run_vario_dt()

def test_ubx_protocol_and_parser():
    run_ubx_parser()

def test_lk8ex1_nmea_protocol_formatter():
    run_lk8ex1_formatter()

def test_vario_engine_iir_filter_and_step_response():
    run_vario_engine()

def test_gnss_flight_state_machine_and_qnh_calibration():
    run_gnss_processor()

def test_battery_monitor_soc_lut_and_filter():
    run_battery_monitor()

def test_buzzer_audio_cadence_and_tone_algorithm():
    run_buzzer()

def test_bmp390_integer_compensation_math():
    run_bmp390_compensation()

