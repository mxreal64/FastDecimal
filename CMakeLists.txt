cmake_minimum_required(VERSION 3.20)
project(FastDecimal VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CheckCXXCompilerFlag)

option(FASTDECIMAL_BUILD_TESTS "Build FastDecimal unit tests" ON)
option(FASTDECIMAL_BUILD_BENCH "Build FastDecimal benchmarks" ON)
option(FASTDECIMAL_BUILD_SHARED "Build FastDecimal C ABI shared library" ON)
option(FASTDECIMAL_ENABLE_ASAN "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

# Compiler optimization flags
set(BASE_FLAGS "-O3" "-funroll-loops")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    list(APPEND BASE_FLAGS "-ftree-vectorize" "-fno-plt" "-flto=auto" "-Wall" "-Wextra" "-Wpedantic")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    list(APPEND BASE_FLAGS "-fvectorize" "-flto=thin" "-Wall" "-Wextra" "-Wpedantic")
elseif(MSVC)
    list(APPEND BASE_FLAGS "/O2" "/W4")
endif()

# Architecture-specific flags
check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    list(APPEND BASE_FLAGS "-march=native")
endif()

if(FASTDECIMAL_ENABLE_ASAN)
    list(APPEND BASE_FLAGS "-fsanitize=address,undefined" "-fno-omit-frame-pointer")
endif()

# Header-only interface library
add_library(decimallib INTERFACE)
add_library(FastDecimal::FastDecimal ALIAS decimallib)

target_include_directories(decimallib INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(decimallib INTERFACE cxx_std_20)

# C ABI Shared library (used by Rust, Java Panama, C#, and C consumers)
if(FASTDECIMAL_BUILD_SHARED)
    add_library(fastdecimal SHARED interop/fd_capi.cpp)
    target_link_libraries(fastdecimal PRIVATE decimallib)
    target_compile_options(fastdecimal PRIVATE ${BASE_FLAGS} -fPIC)
    if(FASTDECIMAL_ENABLE_ASAN)
        target_link_options(fastdecimal PRIVATE "-fsanitize=address,undefined")
    endif()
    set_target_properties(fastdecimal PROPERTIES
        OUTPUT_NAME "fastdecimal"
        SOVERSION ${PROJECT_VERSION_MAJOR}
        VERSION ${PROJECT_VERSION}
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_NAME_DIR "@rpath"
        MACOSX_RPATH TRUE
    )
    install(TARGETS fastdecimal
        EXPORT FastDecimalTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(FILES interop/fd_capi.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fastdecimal)
endif()

# Installation & CMake Package Export
install(TARGETS decimallib
    EXPORT FastDecimalTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT FastDecimalTargets
    FILE FastDecimalTargets.cmake
    NAMESPACE FastDecimal::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FastDecimal
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/FastDecimalConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FastDecimalConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/FastDecimalConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FastDecimal
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/FastDecimalConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/FastDecimalConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FastDecimal
)

# Pkg-config configuration
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/fastdecimal.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/fastdecimal.pc"
    @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fastdecimal.pc"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# Testing
if(FASTDECIMAL_BUILD_TESTS)
    enable_testing()
    add_executable(tests tests/tests.cpp)
    target_link_libraries(tests PRIVATE decimallib)
    target_compile_options(tests PRIVATE ${BASE_FLAGS})
    if(FASTDECIMAL_ENABLE_ASAN)
        target_link_options(tests PRIVATE "-fsanitize=address,undefined")
    endif()
    add_test(NAME unit_tests COMMAND tests)
endif()

# Benchmarks
if(FASTDECIMAL_BUILD_BENCH)
    add_executable(bench bench/bench.cpp)
    target_link_libraries(bench PRIVATE decimallib)
    target_compile_options(bench PRIVATE ${BASE_FLAGS})
    if(FASTDECIMAL_ENABLE_ASAN)
        target_link_options(bench PRIVATE "-fsanitize=address,undefined")
    endif()
endif()
