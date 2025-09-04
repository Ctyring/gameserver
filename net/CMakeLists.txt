cmake_minimum_required(VERSION 3.14)
project(cfl)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编码设置
add_compile_options("/utf-8")

# 平台分流
if (WIN32 AND MSVC)
    message(STATUS "Configuring for Windows (MSVC)...")
    include(${CMAKE_SOURCE_DIR}/CMakeLists.win.cmake)
elseif (UNIX)
    message(STATUS "Configuring for Linux...")
    include(${CMAKE_SOURCE_DIR}/CMakeLists.linux.cmake)
else ()
    message(FATAL_ERROR "Unsupported platform")
endif ()
