project(jnotest LANGUAGES CXX)

custom_add_library(jnotest)

target_link_libraries(jnotest JNOParser)
