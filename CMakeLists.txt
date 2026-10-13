cmake_minimum_required(VERSION 3.15)
# add_compile_options(-fsanitize=address -g)
# add_link_options(-fsanitize=address -g)

option(BUILD_TESTS "Build tests" ON)
option(BINDINGS "Configure for building Python bindings" OFF)

set(QCORE_LIBRARY_TYPE "SHARED" CACHE STRING "QCore library linking")

if (BINDINGS)
  set(QCORE_LIBRARY_TYPE STATIC)
endif()

# qcore requires at least C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(
    qcore 
    VERSION 1.0
    LANGUAGES CXX
    DESCRIPTION "QCORE: A library for core quantum gate and circuit representation")

# check whether the submodule ``modulename`` is correctly cloned in the ``/extern`` directory.
macro(CHECK_SUBMODULE_PRESENT modulename)
  if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/${modulename}/CMakeLists.txt")
    message(
      FATAL_ERROR
        "${modulename} submodule not cloned properly. \
        Please run `git submodule update --init --recursive` \
        from the main project directory")
  endif()
endmacro()

check_submodule_present(json)

# add main library code
add_subdirectory(src)

# add Python binding code
if(BINDINGS)
    check_submodule_present(pybind11)
    check_submodule_present(pybind11_json)
    add_subdirectory(qcore)
endif()

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

if(BUILD_TESTS)
include(GoogleTest)

# add tests

macro(QCORE_TEST NAME)
add_executable(
        ${NAME}
        ${ARGN}
)
target_link_libraries(
        ${NAME}
        PRIVATE
        GTest::gtest_main
        qcore
)
gtest_discover_tests(${NAME})
endmacro()

qcore_test(
  qcore_test
  test/decompose/Clifford_T.cpp
  test/internal_representation/internal_representation_test.cpp
)
endif()
