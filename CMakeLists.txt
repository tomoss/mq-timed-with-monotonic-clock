cmake_minimum_required(VERSION 3.21)
project(mq_timed_monotonic VERSION 0.1.0 LANGUAGES CXX)

# -----------------------------------------------------------------------------
# 1. Header-Only Library
# -----------------------------------------------------------------------------
add_library(mq_monotonic INTERFACE)

target_include_directories(mq_monotonic INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(mq_monotonic INTERFACE cxx_std_11)

# -----------------------------------------------------------------------------
# 2. Dependencies (librt only on Linux)
# -----------------------------------------------------------------------------
if(UNIX AND NOT APPLE)
    find_library(LIBRT rt)
    if (LIBRT)
        target_link_libraries(mq_monotonic INTERFACE ${LIBRT})
    endif()
endif()

# -----------------------------------------------------------------------------
# 3. Optional Example Program
# -----------------------------------------------------------------------------
option(BUILD_EXAMPLE "Build example program" ON)

if(BUILD_EXAMPLE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/example.cpp")
        add_executable(mq_monotonic_example src/example.cpp)
        target_link_libraries(mq_monotonic_example PRIVATE mq_monotonic)
    else()
        message(STATUS "Example source not found, skipping mq_monotonic_example")
    endif()
endif()

# -----------------------------------------------------------------------------
# 4. Installation
# -----------------------------------------------------------------------------
include(GNUInstallDirs)

# Install the header(s)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install the target for find_package()
install(TARGETS mq_monotonic EXPORT mq_monotonicTargets)

install(EXPORT mq_monotonicTargets
    FILE mq_monotonicConfig.cmake
    # Users will call: target_link_libraries(my_app PRIVATE mq_monotonic::mq_monotonic)
    NAMESPACE mq_monotonic::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mq_monotonic
)

# -----------------------------------------------------------------------------
# 5. Packaging (optional)
# -----------------------------------------------------------------------------
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)