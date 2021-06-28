cmake_minimum_required(VERSION 3.19)
project(fmt)

set(CMAKE_CXX_STANDARD 20)

add_library(fmt fmt.cc fmt.hh)

add_executable(display-test test/display-test.cc)
target_include_directories(display-test PUBLIC .)
add_test(display display-test)
