cmake_minimum_required(VERSION 3.5)

project(JNOParser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG>")

#stand up test
enable_testing()

#if(BUILD_TESTING)
    file(GLOB TARGET_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
    add_library(JNOParser ${TARGET_SOURCES})

    target_include_directories(JNOParser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
    add_subdirectory(jnotest)
#endif()

