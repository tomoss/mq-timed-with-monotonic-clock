#include <mqueue.h>

#include <boost/test/unit_test.hpp>
#include <cerrno>
#include <cstring>
#include <ctime>

#include "mq_monotonic.hpp"

using namespace mq_monotonic;

static constexpr const char* QUEUE = "/mq_monotonic_test";

struct mq_fixture {
  mqd_t mq{-1};

  mq_fixture() {
    mq_unlink(QUEUE);

    mq_attr attr{};
    attr.mq_maxmsg = 1;
    attr.mq_msgsize = 64;

    mq = mq_open(QUEUE, O_CREAT | O_RDWR, 0644, &attr);
    BOOST_REQUIRE(mq != (mqd_t)-1);
  }

  ~mq_fixture() {
    if (mq != (mqd_t)-1) mq_close(mq);
    mq_unlink(QUEUE);
  }
};

BOOST_FIXTURE_TEST_CASE(send_and_receive, mq_fixture) {
  const char msg[] = "hello monotonic";

  timespec ts{};
  clock_gettime(CLOCK_MONOTONIC, &ts);
  ts.tv_sec += 2;

  BOOST_REQUIRE(mq_timedsend_monotonic(mq, msg, sizeof(msg), 0, &ts) == 0);

  char buf[64]{};
  ssize_t n = mq_timedreceive_monotonic(mq, buf, sizeof(buf), nullptr, &ts);

  BOOST_REQUIRE(n > 0);
  BOOST_CHECK_EQUAL(std::strcmp(buf, msg), 0);
}

BOOST_FIXTURE_TEST_CASE(receive_timeout, mq_fixture) {
  timespec ts{};
  clock_gettime(CLOCK_MONOTONIC, &ts);
  ts.tv_sec += 1;

  char buf[64]{};
  ssize_t n = mq_timedreceive_monotonic(mq, buf, sizeof(buf), nullptr, &ts);

  BOOST_CHECK_EQUAL(n, -1);
  BOOST_CHECK_EQUAL(errno, ETIMEDOUT);
}

BOOST_FIXTURE_TEST_CASE(invalid_timeout_returns_einval, mq_fixture) {
  timespec bad{-1, 0};

  char buf[64]{};
  ssize_t n = mq_timedreceive_monotonic(mq, buf, sizeof(buf), nullptr, &bad);

  BOOST_CHECK_EQUAL(n, -1);
  BOOST_CHECK_EQUAL(errno, EINVAL);
}
