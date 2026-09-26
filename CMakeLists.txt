cmake_minimum_required(VERSION 3.24)

set(PROJECT ogi_minimal)
project(${PROJECT} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

set(BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_subdirectory(contrib)
add_subdirectory(ogi)

if(WIN32)
    add_subdirectory(ogi_minimal_cpu_win32_gdi)
    add_subdirectory(ogi_minimal_gpu_win32_wgl)
elseif(LINUX)
    add_subdirectory(ogi_minimal_cpu_linux_x11)
    add_subdirectory(ogi_minimal_gpu_linux_xgl)
elseif(APPLE)
    add_subdirectory(ogi_minimal_cpu_macos_cocoa)
    add_subdirectory(ogi_minimal_gpu_macos_agl)
endif()
