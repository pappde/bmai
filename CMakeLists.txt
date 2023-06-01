# drp022521 - reduced from 3.14 to 3.8 for VS2017 support
# dbl051923 - capture git describe for banner
# dbl053123 - cmake 3.11 adds very useful FetchContent and is supported/available with VS2017

cmake_minimum_required (VERSION 3.11)

set(NAME bmai)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(${NAME})

# Get the latest git tag and short commit hash
execute_process(
    COMMAND git describe --dirty --always --tags
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_definitions("-DGIT_DESCRIBE=\"${GIT_DESCRIBE}\"")

enable_testing()

# to collect only needed source files
set(SOURCE ./src/bmai.cpp ./src/bmai_ai.cpp ./src/player.cpp ./src/stats.cpp)

# some IDEs really need the Headers added to the executable
# to be able to index everything correctly
set(HEADERS ./src/bmai.h ./src/bmai_ai.h ./src/stats.h ./src/logger.h ./src/parser.h ./src/game.h ./src/player.h ./src/die.h ./src/dieindexstack.h)

## Key idea: SEPARATE OUT your main() function into its own file so it can be its
## own executable. Separating out main() means you can add this library to be
## used elsewhere.
add_library(${NAME}_lib ${SOURCE} ${HEADERS})
add_executable(${NAME} ./src/main.cpp)

if(UNIX AND NOT APPLE)
   target_link_libraries(${NAME} ${NAME}_lib -static)
else()
    target_link_libraries(${NAME} ${NAME}_lib)
endif()

# to be able to reference header files
target_include_directories(${NAME} PRIVATE ./src)

# remove ZERO_CHECK build
set(CMAKE_SUPPRESS_REGENERATION true)

# HIDE away ALL_BUILD in VS
# for Xcode see https://gitlab.kitware.com/cmake/cmake/-/issues/21853
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(test)
