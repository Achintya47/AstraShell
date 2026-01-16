cmake_minimum_required(VERSION 3.10)
project(astra_shell LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(astra
    src/main.cpp
    src/shell.cpp
)
