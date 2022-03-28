set(TARGET_NAME Launcher)

add_library(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE Launcher.h Launcher.cpp)

# 私有链接库
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Source/Sketch)
target_link_libraries(${TARGET_NAME} PRIVATE Sketch)