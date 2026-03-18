FROM debian:bookworm

RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    ninja-build \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY CMakeLists.txt .
COPY src/ ./src/
COPY include/ ./include/
COPY tests/ ./tests/

ARG C_COMPILER=gcc
ARG CXX_COMPILER=g++
ARG BUILD_TYPE=Release

RUN cmake -G Ninja -B build -S . \
    -DCMAKE_C_COMPILER=${C_COMPILER} \
    -DCMAKE_CXX_COMPILER=${CXX_COMPILER} \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DCMAKE_CXX_EXTENSIONS=OFF \
    -DBUILD_EXAMPLE=ON \
    -DBUILD_UNIT_TESTS=OFF \
    -DBUILD_INTEGRATION_TESTS=OFF

RUN cmake --build build --config ${BUILD_TYPE}

ENTRYPOINT ["./build/mq_monotonic_example"]