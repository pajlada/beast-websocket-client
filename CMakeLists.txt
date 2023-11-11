cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0087 NEW) # evaluates generator expressions in `install(CODE/SCRIPT)`
cmake_policy(SET CMP0091 NEW) # select MSVC runtime library through `CMAKE_MSVC_RUNTIME_LIBRARY`
include(FeatureSummary)

option(EVENTSUB_BUILD_EXAMPLE "Build example application" OFF)

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_SOURCE_DIR}/cmake"
    )

project(eventsub VERSION 0.1.0)

# Find boost on the system
find_package(Boost REQUIRED OPTIONAL_COMPONENTS json)

# Find OpenSSL on the system
find_package(OpenSSL REQUIRED)

find_package(Threads REQUIRED)

find_library(LIBRT rt)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src)

if (EVENTSUB_BUILD_EXAMPLE)
    add_subdirectory(example)
endif ()

feature_summary(WHAT ALL)
