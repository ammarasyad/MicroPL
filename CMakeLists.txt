cmake_minimum_required(VERSION 3.23)
project(MMIO_CLI)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXE_LINKER_FLAGS "-static")

add_executable(MMIO_CLI main.cpp main.h rwdrv.h)
