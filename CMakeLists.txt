cmake_minimum_required(VERSION 3.21.0)
project(DirectXTutorials)

if(NOT WIN32)
    error("Non-Windows platform is not supported.")
endif()

if(MSVC)
    # 如使用动态链接，输出符号表，即-.exp文件
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    
    # MSVC设置Unicode
    remove_definitions(-D_MBCS)
    add_definitions(-D_UNICODE)
endif()

# 用于设置 RUNTIME_OUTPUT_DIRECTORY
set(MYAPP_BINARY_DIRECTORIES ${CMAKE_SOURCE_DIR}/Binary)
# 用于设置 ARCHIVE_OUTPUT_DIRECTORY
set(MYLIB_LIBRARY_DIRECTORIES ${CMAKE_SOURCE_DIR}/Library)
# 用于设置 LIBRARY_OUTPUT_DIRECTORY (MSVC不需要)
set(MYLIB_BINARY_DIRECTORIES ${CMAKE_SOURCE_DIR}/Binary)

add_subdirectory(Source/Launcher)
add_subdirectory(Source/Sketch)

# 在IDE中，对target使用文件夹分类
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)