### MQ Timed With Monotonic Clock

[![C++17+](https://img.shields.io/badge/dialect-C%2B%2B17%2B-blue)](https://en.cppreference.com/w/cpp/17)
[![Build CI](https://github.com/tomoss/mq-timed-with-monotonic-clock/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/tomoss/mq-timed-with-monotonic-clock/actions/workflows/build.yml)
![platform Linux x86_64](https://img.shields.io/badge/platform-Linux%20x86_64--bit-yellow)
![platform Linux ARM](https://img.shields.io/badge/platform-Linux%20ARM-yellow)

#### POSIX message queue functions with `CLOCK_MONOTONIC` timeout support

The POSIX message queue functions `mq_timedreceive()` and `mq_timedsend()` are limited to **`CLOCK_REALTIME`** for their timeout parameter.

This repository provides two wrapper functions: `mq_timedreceive_monotonic()` and `mq_timedsend_monotonic()`, which use **`CLOCK_MONOTONIC`** instead.

The implementation is based primarily on [`poll()`](https://man7.org/linux/man-pages/man2/poll.2.html).

---

#### Integration test for realtime vs monotonic timeout behavior

This repository also includes an integration test that demonstrates the difference between:

- `mq_timedreceive()` using `CLOCK_REALTIME`
- `mq_timedreceive_monotonic()` using `CLOCK_MONOTONIC`

The test changes the Linux system wall clock while both modes are waiting, showing that:

- the `mq_timedreceive()` timeout is affected by wall-clock changes
- the `mq_timedreceive_monotonic()` timeout remains stable

See [tests/integration/README.md](tests/integration/README.md) for details about what the test verifies and how to run it.