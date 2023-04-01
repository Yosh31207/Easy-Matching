cmake_minimum_required(VERSION 3.0.0)

project(easy_match CXX)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(GTEST_URL "https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip" CACHE STRING "GTest download URL")
include(FetchContent)
FetchContent_Declare(
    googletest
    URL ${GTEST_URL}
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
FetchContent_GetProperties(googletest)

set(GTEST_INCLUDE_DIR ${gtest_SOURCE_DIR}/include)
include_directories(${GTEST_INCLUDE_DIR})

set(TEST_APP easy_match_test)
add_executable(${TEST_APP})
target_sources(${TEST_APP}
  PRIVATE
    easy_match_test.cpp
)

set_target_properties(${TEST_APP}
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(${TEST_APP}
  PRIVATE
    ../include
)

target_compile_features(${TEST_APP}
  PRIVATE
    cxx_std_17
)
target_compile_options(${TEST_APP}
  PRIVATE
    -Wall
    -Wextra
)

target_link_libraries(${TEST_APP}
    gtest
    gtest_main
)
