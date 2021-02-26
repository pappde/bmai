cmake_minimum_required(VERSION 3.14)

project(bmai)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# to be able to reference header files
include_directories(./src)

# to collect only needed source files
set(SOURCE ./src/bmai.cpp ./src/bmai_ai.cpp ./src/bmai_player.cpp)

add_executable(${PROJECT_NAME} ${SOURCE})
