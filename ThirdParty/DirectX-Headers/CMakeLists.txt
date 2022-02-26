# 获取d3dx12.h
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include/d3dx12.h)
    file(DOWNLOAD
        https://raw.githubusercontent.com/microsoft/DirectX-Headers/v1.4.9/include/directx/d3dx12.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/d3dx12.h
        SHOW_PROGRESS)
endif()

set(TARGET_NAME DirectX-Headers)

add_library(${TARGET_NAME} INTERFACE)
target_sources(${TARGET_NAME} PRIVATE include/d3dx12.h)
target_include_directories(${TARGET_NAME} INTERFACE include)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER ThirdParty)