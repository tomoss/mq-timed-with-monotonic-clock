#include <boost/test/unit_test.hpp>
#include <climits>
#include <ctime>

#include "mq_monotonic.hpp"

using namespace mq_monotonic;

BOOST_AUTO_TEST_CASE(timeout_validation) {
  timespec ts{1, 0};
  BOOST_CHECK(is_timetout_valid(&ts));

  ts.tv_nsec = NANOS_PER_SEC - 1;
  BOOST_CHECK(is_timetout_valid(&ts));

  ts.tv_nsec = NANOS_PER_SEC;
  BOOST_CHECK(!is_timetout_valid(&ts));

  ts.tv_nsec = -1;
  BOOST_CHECK(!is_timetout_valid(&ts));

  ts.tv_sec = -1;
  ts.tv_nsec = 0;
  BOOST_CHECK(!is_timetout_valid(&ts));

  BOOST_CHECK(!is_timetout_valid(nullptr));
}

BOOST_AUTO_TEST_CASE(delta_time_basic_cases) {
  timespec now{10, 500'000'000};
  timespec later{12, 500'000'000};

  int ms = calculate_delta_time_ms(later, now);
  BOOST_CHECK_EQUAL(ms, 2000);
}

BOOST_AUTO_TEST_CASE(delta_time_nsec_borrow) {
  timespec now{10, 900'000'000};
  timespec later{11, 100'000'000};

  int ms = calculate_delta_time_ms(later, now);
  BOOST_CHECK_EQUAL(ms, 200);
}

BOOST_AUTO_TEST_CASE(delta_time_expired) {
  timespec now{10, 0};
  timespec past{9, 999'000'000};

  int ms = calculate_delta_time_ms(past, now);
  BOOST_CHECK_EQUAL(ms, 0);
}

BOOST_AUTO_TEST_CASE(delta_time_clamped_to_int_max) {
  timespec now{0, 0};
  timespec far{LLONG_MAX / MILLIS_PER_SEC, 0};

  int ms = calculate_delta_time_ms(far, now);
  BOOST_CHECK_EQUAL(ms, INT_MAX);
}
