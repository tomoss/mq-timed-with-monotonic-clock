cmake_minimum_required(VERSION 3.21)
project(mq_timed_monotonic VERSION 0.1.0)

add_executable(mq_timed_monotonic test.cpp)

find_library(LIBRT rt) 
if(LIBRT)
    target_link_libraries(mq_timed_monotonic ${LIBRT})
endif()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})