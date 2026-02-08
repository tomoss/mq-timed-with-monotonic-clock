find_package(Boost REQUIRED)

add_executable(mq_monotonic_tests
    test_main.cpp
    test_helpers.cpp
    test_mq_ops.cpp
)

target_link_libraries(mq_monotonic_tests
    PRIVATE
    mq_monotonic
    rt
)

target_include_directories(mq_monotonic_tests
    PRIVATE
    ${Boost_INCLUDE_DIRS}
)

add_test(NAME mq_monotonic_tests COMMAND mq_monotonic_tests)
