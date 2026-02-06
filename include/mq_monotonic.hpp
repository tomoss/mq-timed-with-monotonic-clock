#ifndef MQ_MONOTONIC_H
#define MQ_MONOTONIC_H

#include <mqueue.h>
#include <poll.h>
#include <cerrno>
#include <climits>
#include <ctime>

namespace mq_monotonic {

// Time conversion constants
constexpr long NANOS_PER_SEC = 1000000000L;
constexpr long long MILLIS_PER_SEC = 1000LL;
constexpr long long NANOS_PER_MILLI = 1000000LL;

// Validates the structure of a timespec, not whether the deadline is in the
// future.
static bool is_timetout_valid(const timespec* abs_timeout) {
  if (abs_timeout == nullptr) {
    return false;
  }

  if (abs_timeout->tv_sec < 0) {
    return false;
  }

  if (abs_timeout->tv_nsec < 0 || abs_timeout->tv_nsec >= NANOS_PER_SEC) {
    return false;
  }

  return true;
}

// Computes (abs_timeout - time_current) in milliseconds.
// Returns:
//    ms > 0  → amount of time to wait
//    ms == 0 → deadline has expired (or is exactly now)
static int calculate_delta_time_ms(const timespec& abs_timeout,
                                   const timespec& time_current) {
  long sec = abs_timeout.tv_sec - time_current.tv_sec;
  long nsec = abs_timeout.tv_nsec - time_current.tv_nsec;

  // Normalize: ensure 0 <= nsec < 1e9
  if (nsec < 0) {
    sec -= 1;
    nsec += NANOS_PER_SEC;
  }

  // If resulting seconds are negative → deadline already expired.
  if (sec < 0) {
    return 0;
  }

  long long ms = sec * MILLIS_PER_SEC + nsec / NANOS_PER_MILLI;

  if (ms < 0) {
    ms = 0;
  } else if (ms > INT_MAX) {
    ms = INT_MAX;
  }

  return static_cast<int>(ms);
}

static ssize_t mq_timedreceive_monotonic(mqd_t mqdes, char* msg_ptr,
                                         size_t msg_len, unsigned int* msg_prio,
                                         const struct timespec* abs_timeout) {
  if (!is_timetout_valid(abs_timeout)) {
    errno = EINVAL;
    return -1;
  }

  // Non-blocking probe using zero timeout.
  timespec zero_timeout = {0, 0};
  ssize_t ret;

  while (true) {
    errno = 0;
    ret = mq_timedreceive(mqdes, msg_ptr, msg_len, msg_prio, &zero_timeout);

    /* SUCCES case */
    if (ret >= 0) {
      return ret;
    }

    // If it's a real error (like invalid buffer), return immediately.
    if (errno != ETIMEDOUT && errno != EAGAIN) {
      return -1;
    }

    timespec time_current = {0, 0};
    if (clock_gettime(CLOCK_MONOTONIC, &time_current) != 0) {
      return -1;  // clock_gettime failure
    }

    int delta_ms = calculate_delta_time_ms(*abs_timeout, time_current);
    if (0 == delta_ms) {
      errno = ETIMEDOUT;
      return -1;
    }

    pollfd fdset[1];
    fdset[0].fd = static_cast<int>(mqdes);
    fdset[0].events = POLLIN;
    fdset[0].revents = 0;

    int rc;
    do {
      rc = poll(fdset, 1, delta_ms);
    } while (rc < 0 && errno == EINTR);

    if (rc < 0) {
      return -1;
    }

    if (rc == 0) {
      errno = ETIMEDOUT;
      return -1;
    }

    // rc > 0: POLLIN may be active → loop back and try mq_timedreceive() again.
  }
}

static int mq_timedsend_monotonic(mqd_t mqdes, const char* msg_ptr,
                                  size_t msg_len, unsigned msg_prio,
                                  const struct timespec* abs_timeout) {
  if (!is_timetout_valid(abs_timeout)) {
    errno = EINVAL;
    return -1;
  }

  // Non-blocking probe using zero timeout.
  timespec zero_timeout = {0, 0};
  ssize_t ret;

  while (true) {
    errno = 0;
    ret = mq_timedsend(mqdes, msg_ptr, msg_len, msg_prio, &zero_timeout);

    /* SUCCES case */
    if (ret >= 0) {
      return ret;
    }

    // If it's a real error (like invalid buffer), return immediately.
    if (errno != ETIMEDOUT && errno != EAGAIN) {
      return -1;
    }

    timespec time_current = {0, 0};
    if (clock_gettime(CLOCK_MONOTONIC, &time_current) != 0) {
      return -1;  // clock_gettime failure
    }

    int delta_ms = calculate_delta_time_ms(*abs_timeout, time_current);
    if (0 == delta_ms) {
      errno = ETIMEDOUT;
      return -1;
    }

    pollfd fdset[1];
    fdset[0].fd = static_cast<int>(mqdes);
    fdset[0].events = POLLOUT;
    fdset[0].revents = 0;

    int rc;
    do {
      rc = poll(fdset, 1, delta_ms);
    } while (rc < 0 && errno == EINTR);

    if (rc < 0) {
      return -1;
    }

    if (rc == 0) {
      errno = ETIMEDOUT;
      return -1;
    }

    // rc > 0: POLLOUT may be active → loop back and try mq_timedsend() again.
  }
}

}  // namespace mq_monotonic

#endif  // MQ_MONOTONIC_H
