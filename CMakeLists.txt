# drp022521 - reduced from 3.14 to 3.8 for VS2017 support
# dbl051923 - capture git describe for banner
# dbl053123 - cmake 3.11 adds FetchContent and is supported/available with VS2017
# dbl100324 - cmake 3.19 and c++17 targeting VS2022

cmake_minimum_required (VERSION 3.19)

set(NAME bmai)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE) # MacOS Universal binary

project(${NAME} VERSION 3.1)


if(NOT DEFINED GIT_DESCRIBE)
    execute_process(
        COMMAND git describe --dirty --always --tags --match=v[0-9]*
        OUTPUT_VARIABLE GIT_DESCRIBE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

add_definitions("-DGIT_DESCRIBE=\"${GIT_DESCRIBE}\"")

enable_testing()

# to collect only needed source files
set(SOURCE ./src/bmai.cpp ./src/bmai_ai.cpp ./src/player.cpp ./src/stats.cpp)

# some IDEs need Headers added to the executable for indexing
set(HEADERS ./src/bmai.h ./src/bmai_ai.h ./src/stats.h ./src/logger.h ./src/parser.h ./src/game.h ./src/player.h ./src/die.h ./src/dieindexstack.h)

## Key idea: SEPARATE OUT main() function to its own executable.
## And contain the application to a library
## allowing you can add this library elsewhere, such as testing

add_library(${NAME}_lib ${SOURCE} ${HEADERS})
add_executable(${NAME} ./src/main.cpp)

if(WIN32)
    target_link_libraries(${NAME} PRIVATE ${NAME}_lib -static)
else()
    target_link_libraries(${NAME} PRIVATE ${NAME}_lib)
endif()

# to be able to reference header files
target_include_directories(${NAME} PRIVATE ./src)

# remove ZERO_CHECK build
set(CMAKE_SUPPRESS_REGENERATION true)

# HIDE away ALL_BUILD in VS
# for Xcode see https://gitlab.kitware.com/cmake/cmake/-/issues/21853
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(test)
