cmake_minimum_required(VERSION 3.20)
project(decimallib VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags with compiler detection
set(BASE_FLAGS "-O3" "-funroll-loops")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    list(APPEND BASE_FLAGS "-ftree-vectorize" "-fno-plt" "-flto=auto")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    list(APPEND BASE_FLAGS "-fvectorize" "-flto=thin")
endif()

# Architecture-specific flags (-march=native only where supported, avoid breaking AppleClang / cross)
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    list(APPEND BASE_FLAGS "-march=native")
endif()

# Header-only interface
add_library(decimallib INTERFACE)
target_include_directories(decimallib INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(decimallib INTERFACE cxx_std_20)

# C ABI Shared library (always built by cmake so rust/java/csharp find it)
add_library(fastdecimal SHARED interop/fd_capi.cpp)
target_link_libraries(fastdecimal PRIVATE decimallib)
target_compile_options(fastdecimal PRIVATE ${BASE_FLAGS} -fPIC)
set_target_properties(fastdecimal PROPERTIES
    OUTPUT_NAME "fastdecimal"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_NAME_DIR "@rpath"
    MACOSX_RPATH TRUE
)

# Benchmarks
add_executable(bench bench/bench.cpp)
target_link_libraries(bench PRIVATE decimallib)
target_compile_options(bench PRIVATE ${BASE_FLAGS})

# Tests
add_executable(tests tests/tests.cpp)
target_link_libraries(tests PRIVATE decimallib)
target_compile_options(tests PRIVATE ${BASE_FLAGS})
