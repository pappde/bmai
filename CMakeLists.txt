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

# to collect only needed source files
set(SOURCE
        #src/bmai.cpp this file is references below in add_executable

        src/BMC_AI.cpp
        src/BMC_AI_Maximize.cpp
        src/BMC_AI_MaximizeOrRandom.cpp
        src/BMC_BMAI.cpp
        src/BMC_BMAI3.cpp
        src/BMC_Die.cpp
        src/BMC_DieData.cpp
        src/BMC_DieIndexStack.cpp
        src/BMC_Game.cpp
        src/BMC_Logger.cpp
        src/BMC_Move.cpp
        src/BMC_Parser.cpp
        src/BMC_Player.cpp
        src/BMC_QAI.cpp
        src/BMC_RNG.cpp
        src/BMC_Stats.cpp
)

# some IDEs need Headers added to the executable for indexing
set(HEADERS
        src/bmai.h
        src/bmai_ai.h

        src/BMC_AI.h
        src/BMC_AI_Maximize.h
        src/BMC_AI_MaximizeOrRandom.h
        src/BMC_BitArray.h
        src/BMC_BMAI.h
        src/BMC_BMAI3.h
        src/BMC_Die.h
        src/BMC_DieData.h
        src/BMC_DieIndexStack.h
        src/BMC_Game.h
        src/BMC_Logger.h
        src/BMC_Man.h
        src/BMC_Move.h
        src/BMC_Parser.h
        src/BMC_Player.h
        src/BMC_QAI.h
        src/BMC_RNG.h
        src/BMC_Stats.h
)

## Key idea: SEPARATE OUT main() function to its own executable.
## And contain the application to a library
## allowing you can add this library elsewhere, such as testing

add_library(${NAME}_lib ${SOURCE} ${HEADERS})
add_executable(${NAME} ./src/bmai.cpp)

# Add precompiled header
target_precompile_headers(${NAME}_lib PRIVATE ./src/bmai.h)

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

enable_testing()

add_subdirectory(test)
