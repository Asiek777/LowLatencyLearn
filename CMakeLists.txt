cmake_minimum_required(VERSION 3.0)

project(LowLatencyLearn)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-std=c++2a -Wall -Wextra -Wpedantic")
set(CMAKE_VERBOSE_MAKEFILE on)

file(GLOB SOURCES "*.cpp")

include_directories(${PROJECT_SOURCE_DIR})

add_library(libcommon STATIC ${SOURCES})

list(APPEND LIBS libcommon)
list(APPEND LIBS pthread)

add_executable(main main.cpp)
target_link_libraries(main PUBLIC ${LIBS})

add_executable(thread_example chapter4/thread_example.cpp)
target_link_libraries(thread_example PUBLIC ${LIBS})

add_executable(mem_pool_example chapter4/mem_pool_example.cpp)
target_link_libraries(mem_pool_example PUBLIC ${LIBS})

add_executable(lf_queue_example chapter4/lf_queue_example.cpp)
target_link_libraries(lf_queue_example PUBLIC ${LIBS})

add_executable(logging_example chapter4/logging_example.cpp)
target_link_libraries(logging_example PUBLIC ${LIBS})

#add_executable(socket_example socket_example.cpp)
#target_link_libraries(socket_example PUBLIC ${LIBS})