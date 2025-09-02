cmake_minimum_required(VERSION 3.0)

project(LowLatencyLearn)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-std=c++2a -Wall -Wextra -Wpedantic")
set(CMAKE_VERBOSE_MAKEFILE on)

add_subdirectory(common)
add_subdirectory(chapter4)

file(GLOB SOURCES "*.cpp" "common/*.cpp")

list(APPEND LIBS libcommon)
list(APPEND LIBS pthread)

add_executable(main main.cpp)
target_link_libraries(main PUBLIC ${LIBS})

