cmake_minimum_required (VERSION 3.19)

project(bmai_tests)

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()


##
# External Dependencies
##

include(FetchContent)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
        GIT_TAG main
)
# For VS/Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Explicitly list the test source files
set(TEST_SOURCES
        BMAI3Tests.cpp
        LegacyFunctions.cpp
        PlayerTest.cpp
        SkillTest.cpp
        testutils.h
        DemoTest.cpp
)

add_executable(bmai_tests ${TEST_SOURCES})

if(APPLE)
    target_link_libraries(bmai_tests PRIVATE bmai_lib GTest::gtest_main)
else()
    target_link_libraries(bmai_tests PRIVATE bmai_lib GTest::gtest_main -static)
endif()

##
# Auto-Discover Tests
##

include(GoogleTest)
gtest_discover_tests(bmai_tests DISCOVERY_MODE PRE_TEST)
