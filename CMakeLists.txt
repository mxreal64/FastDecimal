cmake_minimum_required(VERSION 3.20)
project(decimallib VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Detect CPU and set maximum vectorization flags ──────────────────────────
include(CheckCXXCompilerFlag)

# Aggressive optimization
set(BASE_FLAGS "-O3 -march=native -mtune=native -funroll-loops -ffast-math -fno-rounding-math")
set(BASE_FLAGS "${BASE_FLAGS} -fomit-frame-pointer -fstrict-aliasing")
set(BASE_FLAGS "${BASE_FLAGS} -ftree-vectorize")
set(BASE_FLAGS "${BASE_FLAGS} -fno-plt -fno-semantic-interposition")
# LTO
set(BASE_FLAGS "${BASE_FLAGS} -flto=auto -fuse-linker-plugin")

# Profile-guided optimization hint (optional, users can enable)
option(ENABLE_PGO_GENERATE "Build with PGO instrumentation" OFF)
option(ENABLE_PGO_USE      "Build with PGO data"            OFF)
if(ENABLE_PGO_GENERATE)
    set(BASE_FLAGS "${BASE_FLAGS} -fprofile-generate")
endif()
if(ENABLE_PGO_USE)
    set(BASE_FLAGS "${BASE_FLAGS} -fprofile-use -fprofile-correction")
endif()

set(CMAKE_CXX_FLAGS_RELEASE "${BASE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# ── Library (header-only, but provide a compiled companion) ─────────────────
add_library(decimallib INTERFACE)
target_include_directories(decimallib INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(decimallib INTERFACE cxx_std_20)

# ── Benchmarks ───────────────────────────────────────────────────────────────
add_executable(bench bench/bench.cpp)
target_link_libraries(bench PRIVATE decimallib)
target_compile_options(bench PRIVATE
    -O3 -march=native -mtune=native -funroll-loops -ffast-math
    -fomit-frame-pointer -fno-plt
)

# ── Tests ────────────────────────────────────────────────────────────────────
add_executable(tests tests/tests.cpp)
target_link_libraries(tests PRIVATE decimallib)
target_compile_options(tests PRIVATE -O2 -march=native)

# ── Install ──────────────────────────────────────────────────────────────────
install(DIRECTORY include/ DESTINATION include)
install(TARGETS decimallib EXPORT decimallibTargets)
install(EXPORT decimallibTargets
    FILE decimallibTargets.cmake
    NAMESPACE fd::
    DESTINATION lib/cmake/decimallib
)
