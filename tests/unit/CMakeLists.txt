find_package(Boost CONFIG REQUIRED)

add_executable(mq_monotonic_unit_tests
    test_main.cpp
    test_helpers.cpp
    test_mq_ops.cpp
)

target_link_libraries(mq_monotonic_unit_tests
    PRIVATE
    mq_monotonic
    rt
)

target_include_directories(mq_monotonic_unit_tests
    PRIVATE
    Boost::headers
)

add_test(NAME mq_monotonic_unit_tests COMMAND mq_monotonic_unit_tests)
