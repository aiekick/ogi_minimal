cmake_minimum_required(VERSION 3.24)

set(PROJECT ogi_minimal)
project(${PROJECT} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

set(BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_subdirectory(ogi)
add_subdirectory(ogi_minimal_cpu_gdi)
