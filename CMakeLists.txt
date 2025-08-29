cmake_minimum_required(VERSION 3.25)

# Project
project(tank_sim C CXX ASM)
include(FetchContent)

# Standards
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

# CMake Options
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(FETCHCONTENT_QUIET OFF)

# Add sources
add_subdirectory(src)