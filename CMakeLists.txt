cmake_minimum_required(VERSION 3.21)
project(mq_timed_monotonic VERSION 0.1.0 LANGUAGES CXX)

add_library(mq_monotonic INTERFACE)
add_library(mq_monotonic::mq_monotonic ALIAS mq_monotonic)

target_include_directories(mq_monotonic INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(mq_monotonic INTERFACE cxx_std_17)

if(UNIX AND NOT APPLE)
    target_link_libraries(mq_monotonic INTERFACE rt)
endif()

option(BUILD_EXAMPLE "Build example program" ON)
option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_EXAMPLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/example.cpp")
    add_executable(mq_monotonic_example src/example.cpp)
    target_link_libraries(mq_monotonic_example PRIVATE mq_monotonic)
endif()

if(BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
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
