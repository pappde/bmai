# drp022521 - reduced from 3.14 to 3.8 for VS2017 support
# dbl051923 - capture git describe for banner
# dbl053123 - cmake 3.11 adds FetchContent and is supported/available with VS2017
# dbl100324 - cmake 3.19 and c++17 targeting VS2022

cmake_minimum_required (VERSION 3.19)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE) # MacOS Universal binary

project(bmai VERSION 3.1)

if(NOT DEFINED GIT_DESCRIBE)
    find_package(Git)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --dirty --always --tags --match=v[0-9]*
        OUTPUT_VARIABLE GIT_DESCRIBE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

add_definitions("-DGIT_DESCRIBE=\"${GIT_DESCRIBE}\"")

# to collect only needed source files
set(SOURCE
       # src/bmai_lib.cpp

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
        src/bmai_lib.h
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

## Key idea: SEPARATE OUT main() function to its own bmai executable.
## And contain the orimary app logic to a library, bmai_lib
## then you can add this lib elsewhere, such as testing

add_library(bmai_lib ${SOURCE} ${HEADERS})
add_executable(bmai src/bmai.cpp)

if(WIN32)
    target_link_libraries(bmai PRIVATE bmai_lib -static)
else()
    target_link_libraries(bmai PRIVATE bmai_lib)
endif()

# to be able to reference header files
target_include_directories(bmai PRIVATE ./src)

# remove ZERO_CHECK build
set(CMAKE_SUPPRESS_REGENERATION true)

# HIDE away ALL_BUILD in VS
# for Xcode see https://gitlab.kitware.com/cmake/cmake/-/issues/21853
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

enable_testing()

add_subdirectory(test)
