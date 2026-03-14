## Python integration test driver

This Python script (`test_time_change.py`) runs the `mq_integration_test` helper binary in both modes:

- `mq_timedreceive`
- `mq_timedreceive_monotonic`

It then changes the system wall clock while both processes are blocked waiting for timeout.

Its purpose is to demonstrate the behavioral difference between:

- `mq_timedreceive()` using `CLOCK_REALTIME`
- `mq_timedreceive_monotonic()` using `CLOCK_MONOTONIC`

---

## What it tests

This test verifies that changing the Linux system wall clock affects the two timeout modes differently.

### `mq_timedreceive`

This mode uses `mq_timedreceive()` with a timeout based on `CLOCK_REALTIME`.

If the system time is moved **backward** while the process is waiting, the timeout is delayed.

### `mq_timedreceive_monotonic`

This mode uses `mq_timedreceive_monotonic()` with a timeout based on `CLOCK_MONOTONIC`.

If the system time is moved **backward**, the timeout is **not** affected, because monotonic time does not follow wall-clock changes.

---

## Expected result

When the Python script runs both processes and then shifts the system clock backward:

- the process using **`mq_timedreceive`** should take noticeably longer to time out
- the process using **`mq_timedreceive_monotonic`** should still time out close to the original timeout

In other words, the test demonstrates that:

- `mq_timedreceive()` is affected by wall-clock changes
- `mq_timedreceive_monotonic()` is stable against wall-clock changes

---

## Build the integration test binary

> **Warning**
> This test changes the **global system time**, so it should only be run in a safe environment.

Build the project with integration tests enabled(`-DBUILD_INTEGRATION_TESTS=ON`) or build the integration test target directly:

```bash
cmake --build build --target mq_integration_test
```

Then run the script:
```bash
python3 test_time_change.py
```

**Note:** `sudo` privileges are required to change the system time.

