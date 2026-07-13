cmake_minimum_required(VERSION 3.20)
project(rewrite_vm LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.6.2
)
FetchContent_MakeAvailable(CLI11)

add_executable(rewrite_vm
    src/main.cpp
    src/vm.c
)

target_include_directories(rewrite_vm PRIVATE include)

target_link_libraries(rewrite_vm PRIVATE CLI11::CLI11)
target_compile_options(rewrite_vm PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-g>
)
