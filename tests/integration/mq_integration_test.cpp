#include "mq_monotonic.hpp"

#include <array>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <expected>
#include <iostream>

using mq_monotonic::mq_timedreceive_monotonic;

constexpr const char* QUEUE_NAME = "/mq-test-integration";
constexpr long MAX_MESSAGES = 10;
constexpr long MAX_MSG_SIZE = 32;
constexpr int TIMEOUT = 10;

std::expected<timespec, int> deadline_after_seconds(clockid_t p_clock_id, int p_seconds) {
    timespec l_timespec{};
    if (clock_gettime(p_clock_id, &l_timespec) != 0) {
        return std::unexpected(errno);
    }

    l_timespec.tv_sec += p_seconds;
    return l_timespec;
}

template<auto mq_timed_func_type>
int run_integration_test(clockid_t p_clock_id) {
    struct mq_attr l_attr;
    l_attr.mq_flags = 0;
    l_attr.mq_maxmsg = MAX_MESSAGES;
    l_attr.mq_msgsize = MAX_MSG_SIZE;
    l_attr.mq_curmsgs = 0;

    int l_result = 0;
    mqd_t l_mqd = -1;

    do {
        l_mqd = mq_open(QUEUE_NAME, O_RDONLY | O_CREAT | O_EXCL, 0644, &l_attr);
        if (l_mqd == -1) {
            std::cout << "Failed to open queue: " << strerror(errno) << "\n";
            l_result = 1; // failure
            break;
        }

        std::cout << "Queue opened successfully.\n";

        std::array<char, MAX_MSG_SIZE> l_buffer{}; // dummy buffer for receiving messages

        auto l_deadline = deadline_after_seconds(p_clock_id, TIMEOUT);
        if (!l_deadline) {
            std::cout << "clock_gettime failed, errno = " << l_deadline.error() << '\n';
            l_result = 1; // failure
            break;
        }

        timespec l_mq_timeout = *l_deadline;

        std::cout << "Waiting with timeout: " << TIMEOUT << " seconds\n";
        ssize_t l_ret = mq_timed_func_type(l_mqd, l_buffer.data(), l_buffer.size(), nullptr, &l_mq_timeout);

        if (l_ret < 0 && errno != ETIMEDOUT) {
            std::cout << "MQ timedreceive error: " << strerror(errno) << "\n";
            l_result = 1; // failure
            break;
        }

        if (l_ret >= 0) {
            std::cout << "MQ timedreceive unexpectedly received a message.\n";
            l_result = 1; // failure, we expected a timeout
            break;
        }
        std::cout << "MQ timedreceive timed out.\n"; // as expected
        l_result = 0;                                // success
    } while (false);

    if (l_mqd != -1) {
        mq_close(l_mqd);
    }
    mq_unlink(QUEUE_NAME);

    return l_result;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <realtime|monotonic>\n";
        return 1;
    }

    const std::string mode = argv[1];
    if (mode != "realtime" && mode != "monotonic") {
        std::cerr << "Invalid mode. Use: realtime or monotonic\n";
        return 1;
    }

    if (mode == "realtime") {
        return run_integration_test<&mq_timedreceive>(CLOCK_REALTIME);
    }

    if (mode == "monotonic") {
        return run_integration_test<&mq_timedreceive_monotonic>(CLOCK_MONOTONIC);
    }

    return 1; // should never reach here
}