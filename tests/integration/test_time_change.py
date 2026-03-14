from pathlib import Path
import subprocess
import time
from datetime import datetime
from enum import Enum
from collections import namedtuple

TIMEOUT_SECONDS = 10
CLOCK_SHIFT_SECONDS = 15
START_DELAY_SECONDS = 1
EXTRA_TIME_SECONDS = 10

ProcessResult = namedtuple("ProcessResult", ["returncode", "stdout", "stderr"])

class Mode(Enum):
    REALTIME = "mq_timedreceive"
    MONOTONIC = "mq_timedreceive_monotonic"

def get_current_time_str() -> str:
    return subprocess.check_output(
        ["date", "+%Y-%m-%d %H:%M:%S"],
        text=True,
    ).strip()

def set_system_time(new_time: str) -> None:
    subprocess.run(["sudo", "date", "-s", new_time], check=True)


def set_ntp(enabled: bool) -> None:
    value = "true" if enabled else "false"
    subprocess.run(["sudo", "timedatectl", "set-ntp", value], check=True)


def start_child(binary: Path, mode: Mode) -> subprocess.Popen:
    return subprocess.Popen(
        [str(binary), mode],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

def wait_child(proc: subprocess.Popen, mode: Mode, timeout: int) -> ProcessResult:
    stdout, stderr = proc.communicate(timeout=timeout)

    print(f"\n=== {mode.value} process stdout ===\n")
    print(stdout)
    print("======================")

    return ProcessResult(
        returncode=proc.returncode,
        stdout=stdout,
        stderr=stderr,
    )

def main() -> int:
    script_dir = Path(__file__).resolve().parent
    repo_root = script_dir.parent.parent
    binary = repo_root / "build/tests/integration/mq_integration_test"

    if not binary.exists():
        print(f"Binary not found: {binary}")
        return 1

    original_time = get_current_time_str()
    print(f"Original system time: {original_time}")

    try:
        print("Disabling NTP...")
        set_ntp(False)
        ntp_disabled = True

        print("Starting process with mq_timedreceive...")
        realtime_start = time.monotonic()
        realtime_proc = start_child(binary, "queue_realtime")

        print("Starting process with mq_timedreceive_monotonic...")
        monotonic_start = time.monotonic()
        monotonic_proc = start_child(binary, "queue_monotonic")

        # Give both processes time to create the queue and block in receive.
        time.sleep(START_DELAY_SECONDS)

        current = datetime.now()
        shifted = current.timestamp() - CLOCK_SHIFT_SECONDS
        shifted_str = datetime.fromtimestamp(shifted).strftime("%Y-%m-%d %H:%M:%S")

        print(f"Changing system time backward by {CLOCK_SHIFT_SECONDS} seconds")
        print(f"New system time: {shifted_str}")
        set_system_time(shifted_str)

        child_timeout = TIMEOUT_SECONDS + CLOCK_SHIFT_SECONDS + EXTRA_TIME_SECONDS

        print(f"\nWaiting for child processes...")

        monotonic_result: ProcessResult = wait_child(
            monotonic_proc, Mode.MONOTONIC, child_timeout
        )
        monotonic_elapsed = time.monotonic() - monotonic_start

        realtime_result: ProcessResult = wait_child(
            realtime_proc, Mode.REALTIME, child_timeout
        )

        realtime_elapsed = time.monotonic() - realtime_start

        print(f"\nmq_timedreceive_monotonic process exit code: {monotonic_result.returncode}")
        print(f"mq_timedreceive process exit code : {realtime_result.returncode}")
        print(f"mq_timedreceive_monotonic process elapsed : {monotonic_elapsed:.2f}s")
        print(f"mq_timedreceive process elapsed  : {realtime_elapsed:.2f}s")

        if monotonic_result.returncode != 0:
            raise AssertionError("mq_timedreceive_monotonic process failed")

        if realtime_result.returncode != 0:
            raise AssertionError("mq_timedreceive process failed")

        if monotonic_elapsed > TIMEOUT_SECONDS + 3:
            raise AssertionError(
                f"mq_timedreceive_monotonic should stay near {TIMEOUT_SECONDS}s, got {monotonic_elapsed:.2f}s"
            )

        if realtime_elapsed < TIMEOUT_SECONDS + 5:
            raise AssertionError(
                f"mq_timedreceive should be noticeably extended, got {realtime_elapsed:.2f}s"
            )

        if realtime_elapsed <= monotonic_elapsed:
            raise AssertionError(
                "expected mq_timedreceive to be affected more than mq_timedreceive_monotonic"
            )

        print("\nPASS: mq_timedreceive_monotonic process was not affected by system time change, mq_timedreceive was.")
        print(f"mq_timedreceive set timeout: {TIMEOUT_SECONDS}, actual elapsed: {realtime_elapsed:.2f}s")
        print(f"mq_timedreceive_monotonic set timeout: {TIMEOUT_SECONDS}, actual elapsed: {monotonic_elapsed:.2f}s")
        return 0

    finally:
        print("\nRestoring original system time...")
        try:
            set_system_time(original_time)
        except Exception as e:
            print(f"Failed to restore system time: {e}")

        if ntp_disabled:
            print("Re-enabling NTP...")
            try:
                set_ntp(True)
            except Exception as e:
                print(f"Failed to re-enable NTP: {e}")

        for proc, name in (
            (monotonic_proc, "monotonic"),
            (realtime_proc, "realtime"),
        ):
            if proc is not None and proc.poll() is None:
                print(f"Killing still-running process: {name}")
                proc.kill()


if __name__ == "__main__":
    raise SystemExit(main())