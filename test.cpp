#include <iostream>
#include <mqueue.h>
#include <string.h>
#include <unistd.h>
#include <thread>
#include <fcntl.h>
#include <sys/stat.h>
#include <csignal>
#include "mq_monotonic.hpp"

#define QUEUE_NAME "/mq-test"
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE 256

bool running = true;

void signalHandler( int signum )
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    running = false;
}

void client_thread()
{
    mqd_t mqd = mq_open(QUEUE_NAME, O_RDONLY);

    if (mqd == -1)
    {
        perror("mq_open");
        exit(1);
    }

    struct timespec mq_timeout = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &mq_timeout);
    int timeout = 5; // 5 seconds timeout
    mq_timeout.tv_sec = timeout; 

    char buffer[MAX_MSG_SIZE];

    while(running)
    {
        ssize_t ret = mq_monotonic::mq_timedreceive_monotonic(mqd, buffer, MAX_MSG_SIZE, NULL, &mq_timeout);

        if(ret < 0)
        {
            std::cout << "MQ timedreceive error: " << strerror(errno) << "\n"; 
        }
        else
        {
            std::cout << "MQ timedreceive data: " << buffer << "\n";
        }

        clock_gettime(CLOCK_MONOTONIC, &mq_timeout);
        mq_timeout.tv_sec += timeout;
    }
}

int main(int, char**)
{
    std::cout << "Started !\n";
    signal(SIGTERM, signalHandler);

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    /* Create and open a message queue for writing */
    mqd_t mqd = mq_open(QUEUE_NAME, (O_WRONLY | O_CREAT) , (S_IRUSR | S_IWUSR), &attr);

    if (mqd == -1)
    {
        perror ("mq_open");
        exit (1);
    }

    std::thread t(client_thread);

    struct timespec mq_timeout = {0, 0};
    int timeout = 5; // 5 seconds timeout

    bool sleep_test = true;

    while(running)
    {
        clock_gettime(CLOCK_MONOTONIC, &mq_timeout);
        mq_timeout.tv_sec += timeout;
        int ret = mq_monotonic::mq_timedsend_monotonic(mqd, "HELLO", 6, 10, &mq_timeout);
        if(ret < 0)
        {
            std::cout << "MQ timedsend error: " << strerror(errno) << "\n"; 
        }
        else
        {
            std::cout << "MQ timedsend SUCCES\n";
        }

        if(sleep_test)
        {
            sleep(10); // if sleep is 10 mq_receive will timeout
            sleep_test = false;
        }
        else
        {
            sleep(4); // if sleep is 4 mq_receive will receive data
            sleep_test = true;
        }
    }
    
    t.join();
    mq_close (mqd);
    mq_unlink(QUEUE_NAME);

    std::cout << "Stopped !\n";
}