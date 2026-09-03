cmake_minimum_required(VERSION 3.25)
project(exprflow LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(exprflow
    src/exprflow/graph/graph.cpp
    src/exprflow/executor.cpp
)
add_library(exprflow::exprflow ALIAS exprflow)

target_include_directories(exprflow PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(exprflow PUBLIC cxx_std_23)

enable_testing()
add_subdirectory(tests/unit)
