set(TARGET_NAME Launcher)

add_library(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE Launcher.h Launcher.cpp)
