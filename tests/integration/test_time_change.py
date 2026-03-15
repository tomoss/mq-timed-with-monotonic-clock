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
        [str(binary), mode.value],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

def wait_child(proc: subprocess.Popen, timeout: int) -> ProcessResult:
    stdout, stderr = proc.communicate(timeout=timeout)

    return ProcessResult(
        returncode=proc.returncode,
        stdout=stdout,
        stderr=stderr,
    )

def print_process_result(result: ProcessResult, mode: Mode) -> None:
    print(f"\n=== {mode.value} ===\n")
    print(result.stdout)
    if result.stderr:
        print("======================")
        print(f"\n{mode.value} process stderr:\n{result.stderr}")
        print("======================")
    print(f"{mode.value} process exit code: {result.returncode}")
    print("\n======================")

def assert_process_success(result: ProcessResult, mode: Mode) -> None:
    if result.returncode != 0:
        raise AssertionError(f"{mode.value} process failed")

def assert_expected_timing(monotonic_elapsed: float, realtime_elapsed: float) -> None:
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

    print(f"\nPASS: {Mode.MONOTONIC.value} process was not affected by system time change, {Mode.REALTIME.value} was.")
    print(f"{Mode.REALTIME.value} set timeout: {TIMEOUT_SECONDS}, actual elapsed: {realtime_elapsed:.2f}s")
    print(f"{Mode.MONOTONIC.value} set timeout: {TIMEOUT_SECONDS}, actual elapsed: {monotonic_elapsed:.2f}s")


def main() -> int:
    script_dir = Path(__file__).resolve().parent
    repo_root = script_dir.parent.parent
    binary = repo_root / "build/tests/integration/mq_integration_test"

    if not binary.exists():
        print(f"Binary not found: {binary}")
        return 1

    original_time = get_current_time_str()
    print(f"Original system time: {original_time}")

    ntp_disabled = False
    monotonic_proc = None
    realtime_proc = None

    try:
        print("Disabling NTP...")
        set_ntp(False)
        ntp_disabled = True

        print("Starting process with mq_timedreceive...")
        realtime_start = time.monotonic()
        realtime_proc = start_child(binary, Mode.REALTIME)

        print("Starting process with mq_timedreceive_monotonic...")
        monotonic_start = time.monotonic()
        monotonic_proc = start_child(binary, Mode.MONOTONIC)

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

        # Sequential waiting is acceptable here because both child processes are already running,
        # and elapsed time is measured from each process start, not from when communicate() begins.
        monotonic_result: ProcessResult = wait_child(monotonic_proc, child_timeout)
        monotonic_elapsed = time.monotonic() - monotonic_start

        realtime_result: ProcessResult = wait_child(realtime_proc, child_timeout)
        realtime_elapsed = time.monotonic() - realtime_start

        for processResult, mode in (
            (monotonic_result, Mode.MONOTONIC),
            (realtime_result, Mode.REALTIME),
        ):
            print_process_result(processResult, mode)

        assert_process_success(monotonic_result, Mode.MONOTONIC)
        assert_process_success(realtime_result, Mode.REALTIME)

        assert_expected_timing(monotonic_elapsed, realtime_elapsed)

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