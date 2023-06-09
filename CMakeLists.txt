cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0087 NEW) # evaluates generator expressions in `install(CODE/SCRIPT)`
cmake_policy(SET CMP0091 NEW) # select MSVC runtime library through `CMAKE_MSVC_RUNTIME_LIBRARY`
include(FeatureSummary)

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_SOURCE_DIR}/cmake"
    )

project(beast-websocket-client VERSION 2.4.3)

# Find boost on the system
# `OPTIONAL_COMPONENTS random` is required for vcpkg builds to link.
# `OPTIONAL` is required, because conan doesn't set `boost_random_FOUND`.
find_package(Boost REQUIRED OPTIONAL_COMPONENTS random json)

# Find OpenSSL on the system
find_package(OpenSSL REQUIRED)

find_package(Threads REQUIRED)

find_library(LIBRT rt)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src)

feature_summary(WHAT ALL)
