cmake_minimum_required(VERSION 3.0)

project(LowLatencyLearn)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-std=c++2a -Wall -Wextra -Wpedantic")
set(CMAKE_VERBOSE_MAKEFILE on)

add_subdirectory(common)
add_subdirectory(exchange)

add_subdirectory(chapter4)

list(APPEND LIBS libcommon)
list(APPEND LIBS libexchange)
list(APPEND LIBS pthread)

file(GLOB SOURCES "*.cpp")

include_directories(${PROJECT_SOURCE_DIR})

add_executable(main main.cpp)
target_link_libraries(main PUBLIC ${LIBS})

