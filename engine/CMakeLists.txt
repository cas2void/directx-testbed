set(TARGET_NAME engine)

add_library(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE 
    src/sketch-base.h src/sketch-base.cpp
    src/launcher.h src/launcher.cpp
)

# 将src目录作头文件路径，PUBLIC将使其他依赖本库的target自动添加此搜索路径
target_include_directories(${TARGET_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)