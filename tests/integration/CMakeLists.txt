add_executable(mq_integration_test
    mq_integration_test.cpp
)

target_link_libraries(mq_integration_test PRIVATE mq_monotonic)

target_compile_features(mq_integration_test PRIVATE cxx_std_23)

target_compile_options(mq_monotonic_example PRIVATE
        -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wnull-dereference
    )
