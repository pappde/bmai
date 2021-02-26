# drp022521 - reduced from 3.14 to 3.8 for VS2017 support
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(bmai)

# to be able to reference header files
include_directories(./src)

# to collect only needed source files
set(SOURCE ./src/bmai.cpp ./src/bmai_ai.cpp ./src/bmai_player.cpp)
set(HEADERS ./src/bmai.h ./src/bmai_ai.h)

add_executable(${PROJECT_NAME} ${SOURCE} ${HEADERS})
