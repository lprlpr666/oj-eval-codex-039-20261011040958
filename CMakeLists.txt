cmake_minimum_required(VERSION 3.13)
project(oj_eval_codex_039 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(code code.cpp)
target_include_directories(code PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
