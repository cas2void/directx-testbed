cmake_minimum_required(VERSION 3.21.0)
project(directx-testbed)

if(NOT WIN32)
    error("Non-Windows platform is not supported.")
endif()

if(NOT MSVC)
    error("Non-MSVC platform is not supported.")
endif()

#
# 通用设置
#
    
# MSVC设置Unicode
remove_definitions(-D_MBCS)
add_definitions(-D_UNICODE)

# Windows环境下如使用动态链接，输出符号表，即-.exp文件，如此无需在代码中标记dllexport
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# 在IDE中，对target使用文件夹分类
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

# 用于设置 RUNTIME_OUTPUT_DIRECTORY
set(MY_BINARY_OUTPUT_DIRECTORIES ${CMAKE_BINARY_DIR}/bin)
# 用于设置 ARCHIVE_OUTPUT_DIRECTORY
set(MY_LIBRARY_OUTPUT_DIRECTORIES ${CMAKE_BINARY_DIR}/lib)

function(set_compile_options _target)
    # MSVC设置警告级别和C++标准的特殊处理
    target_compile_options(${_target} PRIVATE -W4)
    target_compile_options(${_target} PRIVATE /std:c++20)
endfunction()

#
# 引擎
#
set(MY_ENGINE_DIRECTORY ${CMAKE_SOURCE_DIR}/engine)
add_subdirectory(${MY_ENGINE_DIRECTORY})

set_compile_options(engine)

# 输出*.dll/*.pdb的路径
set_target_properties(engine PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${MY_BINARY_OUTPUT_DIRECTORIES})

# 输出*.lib/*.exp的路径
set_target_properties(engine PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${MY_LIBRARY_OUTPUT_DIRECTORIES})

#
# 示例项目
#

# 设置target属性
function(set_app_target_properties _target _group_folder)
    set_compile_options(${_target})

    # 输出*.exe的路径
    set_target_properties(${_target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${MY_BINARY_OUTPUT_DIRECTORIES})
    # set_target_properties(${_target} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${MY_BINARY_OUTPUT_DIRECTORIES})

    # IDE中进行分组
    set_target_properties(${_target} PROPERTIES FOLDER ${_group_folder})
endfunction()

# 列出所有示例文件夹
set(MY_EXAMPLE_DIRECTORY ${CMAKE_SOURCE_DIR}/examples)
file(GLOB EXAMPLE_FOLDERS ${MY_EXAMPLE_DIRECTORY}/*)
foreach(_folder ${EXAMPLE_FOLDERS})
    if(IS_DIRECTORY ${_folder})
        # 先添加子文件夹
        add_subdirectory(${_folder})

        # 然后才能获取该文件夹中的target
        get_property(_sub_targets DIRECTORY ${_folder} PROPERTY BUILDSYSTEM_TARGETS)
        foreach(_target IN LISTS _sub_targets)
            # 仅处理可执行程序类型的target
            get_target_property(_target_type ${_target} TYPE)
            if(${_target_type} STREQUAL EXECUTABLE)
                set_app_target_properties(${_target} Examples)

                get_target_property(TARGET_PATH ${_target} SOURCE_DIR)
                message("Example added: ${TARGET_PATH}")
            endif()
        endforeach()
    endif()
endforeach()

