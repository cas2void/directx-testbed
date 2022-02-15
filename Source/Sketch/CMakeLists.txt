set(TARGET_NAME Sketch)

add_executable(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE DummySketch.cpp)

# MSVC设置警告级别和C++标准的特殊处理
if(MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE -W4)
    target_compile_options(${TARGET_NAME} PRIVATE /std:c++17)
else()
    target_compile_options(${TARGET_NAME} PRIVATE -Wall)
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)
endif()

# MSVC的DLL使用隐式链接，需要指定链接路径
if(MSVC OR NOT BUILD_SHARED_LIBS)
   target_link_directories(${TARGET_NAME} PRIVATE ${MYLIB_LIBRARY_DIRECTORIES})
endif()

# 其他链接库
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Source/Launcher)
target_link_libraries(${TARGET_NAME} PRIVATE Launcher)

set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${MYAPP_BINARY_DIRECTORIES})
if(MSVC)
    set_target_properties(${TARGET_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${MYAPP_BINARY_DIRECTORIES})
endif()