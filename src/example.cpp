#include <fcntl.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <unistd.h>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <array>

#include "mq_monotonic.hpp"

constexpr const char* QUEUE_NAME = "/mq-test";
constexpr long MAX_MESSAGES = 10;
constexpr long MAX_MSG_SIZE = 256;
constexpr unsigned int MSG_PRIO = 0;
constexpr int TIMEOUT = 5;
constexpr int MIN_SLEEP_MS = 3000;
constexpr int MAX_SLEEP_MS = 8000;

std::atomic<bool> running = true;

void signalHandler(int signum) {
  std::cout << "Interrupt signal (" << signum << ") received.\n";
  running.store(false);
}

int random_between(int min_ms, int max_ms) {
  static thread_local std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> dist(min_ms, max_ms);
  return dist(rng);
}

std::string printMonotonic() {
  struct timespec time;
  clock_gettime(CLOCK_MONOTONIC, &time);

  std::ostringstream oss;
  // Format: [   123.456789000]
  // Fixed width (setw) ensures logs align perfectly even as time grows
  oss << "[" << std::setw(10) << std::setfill(' ')
      << time.tv_sec  // Seconds (padding spaces)
      << "." << std::setw(9) << std::setfill('0')
      << time.tv_nsec  // Nanoseconds (padding zeros)
      << "]";

  return oss.str();
}

timespec deadline_after_seconds(int sec) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  ts.tv_sec += sec;
  return ts;
}

void consumer_thread() {
  mqd_t mqd;

  while (running.load()) {
    mqd = mq_open(QUEUE_NAME, O_RDONLY);
    if (mqd != -1) {
      break;
    } else if (errno == ENOENT) {
      std::cout << "Waiting for queue creation...\n";
      sleep(1);
    } else {
      perror("mq_open consumer");
      return;
    }
  }

  timespec mq_timeout;
  std::array<char, MAX_MSG_SIZE + 1> buffer; // +1 for safety null terminator

  while (running.load()) {
    mq_timeout = deadline_after_seconds(TIMEOUT);
    std::cout << printMonotonic() << " Waiting data with timeout: " << TIMEOUT
              << "\n";
    ssize_t ret = mq_monotonic::mq_timedreceive_monotonic(
        mqd, buffer.data(), buffer.size(), NULL, &mq_timeout);

    if (ret < 0) {
      std::cout << printMonotonic()
                << " MQ timedreceive error or timeout: " << std::strerror(errno)
                << "\n";
    } else {
      buffer[ret] = '\0';  // Manually null-terminate the received data
      std::cout << printMonotonic() << " MQ timedreceive data: " << buffer.data()
                << "\n";
    }
  }
}

void publisher_thread() {
  struct mq_attr attr;
  attr.mq_flags = 0;
  attr.mq_maxmsg = MAX_MESSAGES;
  attr.mq_msgsize = MAX_MSG_SIZE;
  attr.mq_curmsgs = 0;

  /* Create and open a message queue for writing */
  mqd_t mqd =
      mq_open(QUEUE_NAME, (O_WRONLY | O_CREAT), (S_IRUSR | S_IWUSR), &attr);

  if (mqd == -1) {
    perror("mq_open publisher");
    return;
  }

  timespec mq_timeout;

  while (running.load()) {
    mq_timeout = deadline_after_seconds(TIMEOUT);

    std::string message = "I like crispy strips";
    std::cout << printMonotonic()
              << " Sending for data with timeout: " << TIMEOUT << "\n";
    int ret = mq_monotonic::mq_timedsend_monotonic(
        mqd, message.c_str(), message.size(), MSG_PRIO, &mq_timeout);
    if (ret < 0) {
      std::cout << printMonotonic()
                << " MQ timedsend error or timeout: " << std::strerror(errno)
                << "\n";
    } else {
      std::cout << printMonotonic()
                << " MQ timedsend succesfully sent the data\n";
    }

    int sleep_time_ms = random_between(MIN_SLEEP_MS, MAX_SLEEP_MS);
    std::cout << printMonotonic() << " Publisher thread sleep for "
              << sleep_time_ms << "ms\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_ms));
  }

  mq_close(mqd);
}

int main(int, char**) {
  std::cout << "Example started !\n";

  signal(SIGTERM, signalHandler);
  signal(SIGINT, signalHandler);

  std::thread pub_t(publisher_thread);
  std::thread sub_t(consumer_thread);

  pub_t.join();
  sub_t.join();

  mq_unlink(QUEUE_NAME);

  std::cout << "Example stopped !\n";
}