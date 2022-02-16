get_filename_component(TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

add_executable(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE DummySketch.cpp)

# 私有链接库
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Source/Launcher)
target_link_libraries(${TARGET_NAME} PRIVATE Launcher)

target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Source/Sketch)
target_link_libraries(${TARGET_NAME} PRIVATE Sketch)