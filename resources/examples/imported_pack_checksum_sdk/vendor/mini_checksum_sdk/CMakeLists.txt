cmake_minimum_required(VERSION 3.20)

project(mini_checksum_sdk LANGUAGES C)

add_library(mini_checksum_sdk STATIC
    src/mini_checksum_sdk.c
)

target_include_directories(mini_checksum_sdk PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(mini_checksum_sdk PRIVATE
    -Wall
    -Wextra
)
