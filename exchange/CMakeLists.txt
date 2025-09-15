set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-std=c++2a -Wall -Wextra -Wpedantic")
set(CMAKE_VERBOSE_MAKEFILE on)

list(APPEND LIBS libcommon)
list(APPEND LIBS pthread)

file(GLOB SOURCES "*.cpp" "*/*.cpp")

include_directories(${PROJECT_SOURCE_DIR})


add_library(libexchange STATIC ${SOURCES})
target_link_libraries(libexchange PUBLIC ${LIBS})
list(APPEND LIBS libexchange)


add_executable(exchange_main exchange_main.cpp)
target_link_libraries(exchange_main PUBLIC ${LIBS})
