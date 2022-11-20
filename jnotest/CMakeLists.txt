file(GLOB TARGET_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(just-test ${TARGET_SOURCES})

set(JustFILEINPUT "syntax.just")
set(DESTINATION_JustFILE "${CMAKE_CURRENT_BINARY_DIR}/${JustFILEINPUT}")
file(CREATE_LINK "${CMAKE_CURRENT_SOURCE_DIR}/${JustFILEINPUT}" ${DESTINATION_JustFILE} SYMBOLIC)

target_link_libraries(just-test libjust)
