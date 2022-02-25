set(TARGET_NAME HelloTriangle)

add_executable(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE SketchApp.cpp)

# 私有链接库
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Source/Launcher)
target_link_libraries(${TARGET_NAME} PRIVATE Launcher)

target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Source/Sketch)
target_link_libraries(${TARGET_NAME} PRIVATE Sketch)

target_link_libraries(${TARGET_NAME} PRIVATE dxgi.lib d3d12.lib d3dcompiler.lib)