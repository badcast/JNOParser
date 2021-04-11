cmake_minimum_required(VERSION 3.5)

project(JNOParser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#stand up test
enable_testing()

function(custom_add_library TARGET)
    file(GLOB TARGET_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
    add_library(${TARGET} ${TARGET_SOURCES})
endfunction()

function(custom_add_exec TARGET)
    file(GLOB TARGET_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
    add_executable(${TARGET} ${TARGET_SOURCES})
endfunction()

#if(BUILD_TESTING)
    custom_add_library(JNOParser)
    target_include_directories(JNOParser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
    add_subdirectory(jnotest)
#endif()

