cmake_minimum_required(VERSION 3.20)
project(rewrite_cpp LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.6.2
)
FetchContent_MakeAvailable(CLI11)

add_executable(rewrite_cpp
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
)

target_include_directories(rewrite_cpp PRIVATE include)

target_link_libraries(rewrite_cpp PRIVATE CLI11::CLI11)

target_compile_options(rewrite_cpp PRIVATE -Wall -Wextra)
