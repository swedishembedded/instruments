# SPDX-License-Identifier: MIT
# Copyright 2022 Martin Schröder <info@swedishembedded.com>
# Consulting: https://swedishembedded.com/consulting
# Training: https://swedishembedded.com/training
# This is a standalone CMake file for use without Zephyr

cmake_minimum_required(VERSION 3.12.2)

project (instruments VERSION 1.0.1)

find_package(SDL2 REQUIRED)

include(CTest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/local/include")

add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(tests)

install(TARGETS instruments
    RUNTIME DESTINATION "/usr/"
    LIBRARY DESTINATION "/usr/"
    PUBLIC_HEADER DESTINATION "/instruments/"
    DESTINATION "/usr/"
)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/instruments DESTINATION include COMPONENT devel)

SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Martin Schröder")
INCLUDE(CPack)
