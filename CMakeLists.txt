cmake_minimum_required(VERSION 3.19)
project(fmt)

set(CMAKE_CXX_STANDARD 20)
string(APPEND CMAKE_CXX_FLAGS_DEBUG " /DNDEBUG")

# Required for CLion indexing and autocompletion to work properly.
add_library(fmt fmt.cc)
target_include_directories(fmt PUBLIC include)
target_compile_options(fmt PRIVATE "/EHsc" "/GR-" "/utf-8")

