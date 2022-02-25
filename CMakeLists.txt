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

# 在IDE中，对target使用文件夹分类
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

function(set_compile_options _target)
    # MSVC设置警告级别和C++标准的特殊处理
    if(MSVC)
        target_compile_options(${_target} PRIVATE -W4)
        target_compile_options(${_target} PRIVATE /std:c++17)
    else()
        target_compile_options(${_target} PRIVATE -Wall)
        target_compile_features(${_target} PRIVATE cxx_std_17)
    endif()
endfunction()

function(set_lib_target_properties _target)
    set_compile_options(${_target})

    set_target_properties(${_target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${MYLIB_BINARY_DIRECTORIES})
    set_target_properties(${_target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${MYLIB_LIBRARY_DIRECTORIES})
    set_target_properties(${_target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${MYLIB_BINARY_DIRECTORIES})
endfunction()

function(set_app_target_properties _target _group_folder)
    set_compile_options(${_target})
    
    # MSVC的DLL使用隐式链接，需要指定链接路径
    if(MSVC OR NOT BUILD_SHARED_LIBS)
        target_link_directories(${_target} PRIVATE ${MYLIB_LIBRARY_DIRECTORIES})
    endif()

    set_target_properties(${_target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${MYAPP_BINARY_DIRECTORIES})
    if(MSVC)
        set_target_properties(${_target} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${MYAPP_BINARY_DIRECTORIES})
    endif()

    set_target_properties(${_target} PROPERTIES FOLDER ${_group_folder})
endfunction()

function(add_lib_in_subdirectory _folder)
    add_subdirectory(${_folder})
    get_property(_sub_targets DIRECTORY ${_folder} PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_target IN LISTS _sub_targets)
        set_lib_target_properties(${_target})
    endforeach()
endfunction()

function(add_app_in_subdirectory _folder _group_folder)
    add_subdirectory(${_folder})
    get_property(_sub_targets DIRECTORY ${_folder} PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_target IN LISTS _sub_targets)
        set_app_target_properties(${_target} ${_group_folder})
    endforeach()
endfunction()

add_lib_in_subdirectory(Source/Launcher)
add_lib_in_subdirectory(Source/Sketch)
add_app_in_subdirectory(Source/Examples/DummySketch Examples)
add_app_in_subdirectory(Source/Examples/GraphicsSamples/HelloWorld Examples/GraphicsSamples)
add_app_in_subdirectory(Source/Examples/GraphicsSamples/HelloTriangle Examples/GraphicsSamples)
