### POSIX Message Queue functions that accept timeout of type CLOCK_MONOTONIC

[![C++11](https://img.shields.io/badge/dialect-C%2B%2B11-blue)](https://en.cppreference.com/w/cpp/11)

[![CI: GCC + Clang](https://github.com/tomoss/mq-timed-with-monotonic-clock/actions/workflows/build.yml/badge.svg)](https://github.com/tomoss/mq-timed-with-monotonic-clock/actions/workflows/build.yml)

![platform Linux x86_64](https://img.shields.io/badge/platform-Linux%20x86_64--bit-yellow)
![platform Linux ARM](https://img.shields.io/badge/platform-Linux%20ARM-yellow)

The POSIX message queue functions:
```
mq_timedreceive
mq_timedsend
```
are limitied to using **CLOCK_REALTIME** as timeout parameter.

Here is an implementation of two wrapper functions for them, named:
```
mq_timedreceive_monotonic
mq_timedsend_monotonic
```
that accept as timeout parameter **CLOCK_MONOTONIC**. 

Implementation was done using Linux API function ```poll()```
