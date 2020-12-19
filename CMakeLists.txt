cmake_minimum_required(VERSION 3.14)

project(bmai)

set(CMAKE_CXX_STANDARD 14)

# to be able to reference header files
include_directories(.)

# to collect only needed source files
set(SOURCE bmai.cpp bmai_ai.cpp bmai_player.cpp)

add_executable(${PROJECT_NAME} ${SOURCE})
