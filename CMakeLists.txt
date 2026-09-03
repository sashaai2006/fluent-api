cmake_minimum_required(VERSION 3.25)
project(dag LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(dag src/dag/graph.cpp)
add_library(dag::dag ALIAS dag)

add_library(executor src/executor/executor.cpp
    include/executor/executor.hpp
    include/executor/block_queue.hpp)
add_library(executor::executor ALIAS executor)

target_include_directories(dag PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(executor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(executor PUBLIC dag::dag)

target_compile_features(dag PUBLIC cxx_std_23)
target_compile_features(executor PUBLIC cxx_std_23)

enable_testing()
add_subdirectory(tests/unit)
