cmake_minimum_required(VERSION 3.20)

project(concurrent_learned_index
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(concurrent_learned_index INTERFACE)

target_include_directories(concurrent_learned_index
    INTERFACE
        ${PROJECT_SOURCE_DIR}/cpp/include
)