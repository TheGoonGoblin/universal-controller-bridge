# SPDX-License-Identifier: MIT
"""Tests a clearly named fake backend, never a hardware compatibility test."""
import os
import subprocess
import sys

exe = sys.argv[1]

def run(mode, args, status, fragment):
    env = dict(os.environ, UCB_TEST_MODE=mode)
    p = subprocess.run([exe, *args], env=env, capture_output=True, text=True, timeout=5)
    assert p.returncode == status, (mode, p.returncode, p.stdout, p.stderr)
    assert fragment in p.stdout + p.stderr, (mode, fragment, p.stdout, p.stderr)

read = ["--read", "1", "2", "1"]
run("", ["--list"], 0, "endpoint=131")
run("", read, 0, "decoded player=2 mask=16384")
run("malformed", read, 3, "valid_reports=0")
run("unplug", read, 1, "LIBUSB_ERROR_NO_DEVICE")
run("open_error", read, 1, "LIBUSB_ERROR_ACCESS")
run("claim_error", read, 1, "LIBUSB_ERROR_BUSY")
run("wrong_interface", read, 1, "No supported active gamepad interface")
run("empty", read, 3, "not found")
run("init_error", ["--list"], 1, "libusb_init")
run("", ["--read", "1", "2", "0"], 2, "Usage")
run("", ["--read", "1", "2", "61"], 2, "Usage")
run("", ["--read", "1", "2", "1x"], 2, "Usage")
print("12 USB CLI fixture scenarios passed (simulated backend)")
