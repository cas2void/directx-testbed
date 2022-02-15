set(TARGET_NAME Launcher)

add_library(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE Launcher.h Launcher.cpp)

# MSVC设置警告级别和C++标准的特殊处理
if(MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE -W4)
    target_compile_options(${TARGET_NAME} PRIVATE /std:c++17)
else()
    target_compile_options(${TARGET_NAME} PRIVATE -Wall)
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)
endif()


set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${MYLIB_BINARY_DIRECTORIES})
set_target_properties(${TARGET_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${MYLIB_LIBRARY_DIRECTORIES})
set_target_properties(${TARGET_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${MYLIB_BINARY_DIRECTORIES})