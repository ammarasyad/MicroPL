cmake_minimum_required(VERSION 3.23)
project(MMIO_CLI VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXE_LINKER_FLAGS "-static")

set(MMIO_VERSION_MAJOR 1)
set(MMIO_VERSION_MINOR 1)
set(MMIO_VERSION_PATCH 0)

configure_file(
        version.h.in ${PROJECT_SOURCE_DIR}/version.h
)

add_executable(MMIO_CLI main.cpp main.h rwdrv.h)
