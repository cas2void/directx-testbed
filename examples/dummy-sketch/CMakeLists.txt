get_filename_component(TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

add_executable(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE src/main.cpp)

set_target_properties(${TARGET_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)

target_link_libraries(${TARGET_NAME} PRIVATE engine)