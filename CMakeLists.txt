cmake_minimum_required(VERSION 3.16)
project(EzCodec VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Threading support (required for ThreadPool on all platforms)
find_package(Threads REQUIRED)

# Core library (all source except main.cpp)
add_library(ezcodec_lib STATIC
    src/Picture.cpp
    src/Quantization.cpp
    src/ThreadPool.cpp
    src/Codec.cpp
    src/EzcFormat.cpp
    third_party/stb/stb_impl.cpp
)

target_include_directories(ezcodec_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_link_libraries(ezcodec_lib PUBLIC Threads::Threads)

# Platform-specific settings
if(MSVC)
    target_compile_definitions(ezcodec_lib PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(ezcodec_lib PRIVATE /W4)
else()
    target_compile_options(ezcodec_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(UNIX)
    target_link_libraries(ezcodec_lib PUBLIC m)
endif()

# Main executable
add_executable(ezcodec src/main.cpp)
target_link_libraries(ezcodec PRIVATE ezcodec_lib)

# Optional: tests
option(EZCODEC_BUILD_TESTS "Build unit tests" OFF)
if(EZCODEC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(TARGETS ezcodec RUNTIME DESTINATION bin)
