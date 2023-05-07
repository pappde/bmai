# drp022521 - reduced from 3.14 to 3.8 for VS2017 support
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(bmai)

# to collect only needed source files
set(SOURCE ./src/bmai.cpp ./src/bmai_ai.cpp ./src/player.cpp ./src/stats.cpp)

# some IDEs really need the Headers added to the executable
# to be able to index everything correctly
set(HEADERS ./src/bmai.h ./src/bmai_ai.h ./src/stats.h ./src/logger.h ./src/parser.h ./src/game.h ./src/player.h ./src/die.h ./src/dieindexstack.h)

add_executable(${PROJECT_NAME} ${SOURCE} ${HEADERS})

target_link_libraries(${PROJECT_NAME} -static)

# to be able to reference header files
target_include_directories(${PROJECT_NAME} PRIVATE ./src)

# remove ZERO_CHECK build
set(CMAKE_SUPPRESS_REGENERATION true)

# HIDE away ALL_BUILD in VS
# for Xcode see https://gitlab.kitware.com/cmake/cmake/-/issues/21853
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
