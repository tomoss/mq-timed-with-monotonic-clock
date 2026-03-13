#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <mqueue.h>
#include <random>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

#include "mq_monotonic.hpp"

constexpr const char* QUEUE_NAME = "/mq-test";
constexpr long MAX_MESSAGES = 10;
constexpr long MAX_MSG_SIZE = 256;
constexpr unsigned int MSG_PRIO = 0;
constexpr int TIMEOUT = 5;
constexpr int MIN_SLEEP_MS = 3000;
constexpr int MAX_SLEEP_MS = 8000;

std::atomic<bool> g_running{true};

void signal_handler(int) {
    const char l_msg[] = "Signal received, stopping...\n";
    // write() is async-signal-safe, unlike std::cout
    write(STDOUT_FILENO, l_msg, sizeof(l_msg) - 1);
    g_running.store(false, std::memory_order_relaxed);
}

bool setup_signal_handlers() {
    struct sigaction l_sa{};
    l_sa.sa_handler = signal_handler;
    sigemptyset(&l_sa.sa_mask);
    sigaddset(&l_sa.sa_mask, SIGTERM);
    l_sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &l_sa, nullptr) == -1)
        return false;

    sigdelset(&l_sa.sa_mask, SIGTERM);
    sigaddset(&l_sa.sa_mask, SIGINT);

    if (sigaction(SIGTERM, &l_sa, nullptr) == -1)
        return false;

    return true;
}

int random_between(int p_min_ms, int p_max_ms) {
    static thread_local std::mt19937 l_rng(std::random_device{}());
    std::uniform_int_distribution<int> l_dist(p_min_ms, p_max_ms);
    return l_dist(l_rng);
}

std::string print_mono_time() {
    struct timespec l_time;
    clock_gettime(CLOCK_MONOTONIC, &l_time);

    std::ostringstream l_oss;
    // Format: [   123.456789000]
    // Fixed width (setw) ensures logs align perfectly even as time grows
    l_oss << "[" << std::setw(10) << std::setfill(' ') << l_time.tv_sec // Seconds (padding spaces)
          << "." << std::setw(9) << std::setfill('0') << l_time.tv_nsec // Nanoseconds (padding zeros)
          << "]";

    return l_oss.str();
}

timespec deadline_after_seconds(int p_seconds) {
    struct timespec l_timespec;
    clock_gettime(CLOCK_MONOTONIC, &l_timespec);
    l_timespec.tv_sec += p_seconds;
    return l_timespec;
}

void consumer_thread() {
    mqd_t l_mqd;

    while (g_running.load()) {
        l_mqd = mq_open(QUEUE_NAME, O_RDONLY);
        if (l_mqd != -1) {
            break;
        } else if (errno == ENOENT) {
            std::cout << "Waiting for queue creation...\n";
            sleep(1);
        } else {
            std::cout << "Error opening queue: " << std::strerror(errno) << "\n";
            return;
        }
    }

    timespec l_mq_timeout;
    std::array<char, MAX_MSG_SIZE + 1> l_buffer{}; // +1 for safety null terminator

    while (g_running.load(std::memory_order_relaxed)) {
        l_mq_timeout = deadline_after_seconds(TIMEOUT);
        std::cout << print_mono_time() << " Waiting data with timeout: " << TIMEOUT << "\n";
        ssize_t l_ret = mq_monotonic::mq_timedreceive_monotonic(l_mqd, l_buffer.data(), l_buffer.size(), nullptr, &l_mq_timeout);

        if (l_ret < 0) {
            std::cout << print_mono_time() << " MQ timedreceive error or timeout: " << std::strerror(errno) << "\n";
        } else {
            l_buffer[static_cast<size_t>(l_ret)] = '\0'; // Manually null-terminate the received data
            std::cout << print_mono_time() << " MQ timedreceive data: " << l_buffer.data() << "\n";
        }
    }
}

void publisher_thread() {
    struct mq_attr l_attr;
    l_attr.mq_flags = 0;
    l_attr.mq_maxmsg = MAX_MESSAGES;
    l_attr.mq_msgsize = MAX_MSG_SIZE;
    l_attr.mq_curmsgs = 0;

    /* Create and open a message queue for writing */
    mqd_t l_mqd = mq_open(QUEUE_NAME, (O_WRONLY | O_CREAT), (S_IRUSR | S_IWUSR), &l_attr);

    if (l_mqd == -1) {
        std::cout << "Error creating/opening queue: " << std::strerror(errno) << "\n";
        return;
    }

    timespec l_mq_timeout;

    while (g_running.load(std::memory_order_relaxed)) {
        l_mq_timeout = deadline_after_seconds(TIMEOUT);

        std::string message = "Te nasti, trudesti si mori";
        std::cout << print_mono_time() << " Sending for data with timeout: " << TIMEOUT << "\n";
        int l_ret = mq_monotonic::mq_timedsend_monotonic(l_mqd, message.c_str(), message.size(), MSG_PRIO, &l_mq_timeout);
        if (l_ret < 0) {
            std::cout << print_mono_time() << " MQ timedsend error or timeout: " << std::strerror(errno) << "\n";
        } else {
            std::cout << print_mono_time() << " MQ timedsend succesfully sent the data\n";
        }

        int l_sleep_time_ms = random_between(MIN_SLEEP_MS, MAX_SLEEP_MS);
        std::cout << print_mono_time() << " Publisher thread sleep for " << l_sleep_time_ms << "ms\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(l_sleep_time_ms));
    }

    mq_close(l_mqd);
}

int main(int, char**) {
    std::cout << "=== Example started ===\n";

    if (!setup_signal_handlers()) {
        std::cerr << "Failed to set up signal handlers\n";
        return 1;
    }

    std::thread l_pub_t(publisher_thread);
    std::thread l_sub_t(consumer_thread);

    l_pub_t.join();
    l_sub_t.join();

    mq_unlink(QUEUE_NAME);

    std::cout << "=== Example stopped ===\n";
    return 0;
}