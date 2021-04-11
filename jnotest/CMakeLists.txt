project(jnotest LANGUAGES CXX)

file(GLOB TARGET_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(jnotest ${TARGET_SOURCES})

target_link_libraries(jnotest JNOParser)
