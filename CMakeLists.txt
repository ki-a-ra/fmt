cmake_minimum_required(VERSION 3.19)
project(fmt)

set(CMAKE_CXX_STANDARD 20)
add_compile_definitions(NDEBUG)

add_library(fmt fmt.cc fmt.hh)
target_compile_options(fmt PRIVATE "/EHsc" "/GR-" "/utf-8")

add_executable(display-test test/display-test.cc)
target_include_directories(display-test PUBLIC .)
add_test(display display-test)

add_executable(new-api test/new-api.cc)
target_include_directories(new-api PUBLIC .)
target_link_libraries(new-api legacy_stdio_definitions.lib)
add_test(new-api new-api)
