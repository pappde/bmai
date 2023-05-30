cmake_minimum_required (VERSION 3.8)

project(${NAME}_tests)

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
enable_testing()

file(GLOB TEST_SOURCES *.cpp *.h)

# Now simply link against gtest or gtest_main as needed. Eg
add_executable(${NAME}_tests ${TEST_SOURCES})
target_link_libraries(${NAME}_tests ${NAME}_lib gtest_main)

include(GoogleTest)
gtest_discover_tests(${NAME}_tests)
