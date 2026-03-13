cmake_minimum_required(VERSION 3.21)
project(mq_timed_monotonic VERSION 0.1.0 LANGUAGES CXX)

add_library(mq_monotonic INTERFACE)
add_library(mq_monotonic::mq_monotonic ALIAS mq_monotonic)

target_include_directories(mq_monotonic INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(mq_monotonic INTERFACE cxx_std_23)

if(UNIX AND NOT APPLE)
    target_link_libraries(mq_monotonic INTERFACE rt)
endif()

option(BUILD_EXAMPLE "Build example program" ON)
option(BUILD_UNIT_TESTS "Build unit tests" ON)
option(BUILD_INTEGRATION_TESTS "Build integration tests" ON)

if(BUILD_EXAMPLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/example.cpp")
    add_executable(mq_monotonic_example src/example.cpp)
    target_link_libraries(mq_monotonic_example PRIVATE mq_monotonic)
    target_compile_options(mq_monotonic_example PRIVATE
        -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wnull-dereference
    )
endif()

if(BUILD_UNIT_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests/unit)
endif()

if(BUILD_INTEGRATION_TESTS)
    add_subdirectory(tests/integration)
endif()

include(GNUInstallDirs)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS mq_monotonic
    EXPORT mq_monotonicTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT mq_monotonicTargets
    FILE mq_monotonicTargets.cmake
    NAMESPACE mq_monotonic::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mq_monotonic
)
