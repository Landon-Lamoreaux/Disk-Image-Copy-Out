cmake_minimum_required(VERSION 3.28)
project(dicpo)

set(CMAKE_CXX_STANDARD 17)

file(GLOB C_SOURCES *.c)

add_executable(dicpo main.cpp
                driver.c)

target_sources(dicpo PRIVATE ${C_SOURCES})