#!/usr/bin/env python3
"""Rebuild, upload, and monitor a PlatformIO target whenever sources change."""

from __future__ import annotations

import os
import shlex
import shutil
import signal
import subprocess
import sys
import time
from pathlib import Path


ROOT = Path.cwd()
ENVIRONMENT = os.getenv("PIO_ENV", "xiao_esp32c3")
PORT = os.getenv("PIO_PORT", "/dev/ttyACM0")
BAUD = os.getenv("PIO_BAUD", "115200")
INTERVAL = float(os.getenv("PIO_WATCH_INTERVAL", "0.35"))
FILTERS = [
    item.strip()
    for item in os.getenv(
        "PIO_MONITOR_FILTERS", "time,esp32_exception_decoder"
    ).split(",")
    if item.strip()
]

WATCH_DIRS = ("src", "include", "lib", "data", "test", "tests")
WATCH_FILES = (
    "platformio.ini",
    "partitions.csv",
    "sdkconfig",
    "sdkconfig.defaults",
)
IGNORED_PARTS = {".git", ".pio", ".vscode", "__pycache__"}


def watched_files() -> list[Path]:
    files: list[Path] = []

    for name in WATCH_FILES:
        path = ROOT / name
        if path.is_file():
            files.append(path)

    for directory_name in WATCH_DIRS:
        directory = ROOT / directory_name
        if not directory.is_dir():
            continue
        for path in directory.rglob("*"):
            if path.is_file() and not IGNORED_PARTS.intersection(path.parts):
                files.append(path)

    return sorted(files)


def snapshot() -> dict[str, tuple[int, int]]:
    state: dict[str, tuple[int, int]] = {}
    for path in watched_files():
        try:
            stat = path.stat()
        except FileNotFoundError:
            continue
        state[str(path)] = (stat.st_mtime_ns, stat.st_size)
    return state


def show(command: list[str]) -> None:
    print(f"\n[pipeline] {shlex.join(command)}", flush=True)


def run_upload() -> int:
    command = [
        "pio",
        "run",
        "--environment",
        ENVIRONMENT,
        "--target",
        "upload",
        "--upload-port",
        PORT,
    ]
    show(command)
    return subprocess.run(command, cwd=ROOT, check=False).returncode


def start_monitor() -> subprocess.Popen[bytes]:
    command = [
        "pio",
        "device",
        "monitor",
        "--port",
        PORT,
        "--baud",
        BAUD,
    ]
    for monitor_filter in FILTERS:
        command.extend(("--filter", monitor_filter))
    show(command)
    return subprocess.Popen(command, cwd=ROOT, start_new_session=True)


def stop_process(process: subprocess.Popen[bytes] | None) -> None:
    if process is None or process.poll() is not None:
        return
    os.killpg(process.pid, signal.SIGINT)
    try:
        process.wait(timeout=3)
    except subprocess.TimeoutExpired:
        os.killpg(process.pid, signal.SIGTERM)
        process.wait(timeout=2)


def wait_for_change(baseline: dict[str, tuple[int, int]]) -> None:
    while snapshot() == baseline:
        time.sleep(INTERVAL)


def settle_changes() -> dict[str, tuple[int, int]]:
    current = snapshot()
    while True:
        time.sleep(INTERVAL)
        latest = snapshot()
        if latest == current:
            return latest
        current = latest


def main() -> int:
    if shutil.which("pio") is None:
        print("error: PlatformIO executable 'pio' was not found", file=sys.stderr)
        return 127
    if not (ROOT / "platformio.ini").is_file():
        print(
            f"error: {ROOT / 'platformio.ini'} does not exist; run this from the project root",
            file=sys.stderr,
        )
        return 2

    print(
        f"[pipeline] watching {ROOT} | env={ENVIRONMENT} | port={PORT} | baud={BAUD}",
        flush=True,
    )

    monitor: subprocess.Popen[bytes] | None = None
    try:
        while True:
            before_build = snapshot()
            result = run_upload()
            after_build = snapshot()

            if after_build != before_build:
                print("[pipeline] files changed during the build; rebuilding", flush=True)
                settle_changes()
                continue

            if result != 0:
                print("[pipeline] upload failed; waiting for a file change", flush=True)
                wait_for_change(after_build)
                settle_changes()
                continue

            monitor = start_monitor()
            while snapshot() == after_build:
                if monitor.poll() is not None:
                    print(
                        "[pipeline] monitor exited; retrying after 1 second",
                        flush=True,
                    )
                    time.sleep(1)
                    monitor = start_monitor()
                time.sleep(INTERVAL)

            print("\n[pipeline] change detected; stopping monitor", flush=True)
            stop_process(monitor)
            monitor = None
            settle_changes()
    except KeyboardInterrupt:
        print("\n[pipeline] stopping", flush=True)
    finally:
        stop_process(monitor)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
