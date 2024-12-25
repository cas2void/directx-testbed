# 下载DirectX-Headers必要文件
set(DIRECTX_HEADERS_FILES 
    include/directx/d3dx12_default.h 
    include/directx/d3dx12_root_signature.h
    include/directx/d3dx12_barriers.h
)

# 定义DirectX Headers的版本和基础URL
set(DIRECTX_HEADERS_VERSION "v1.614.1")
set(DIRECTX_HEADERS_BASE_URL "https://raw.githubusercontent.com/microsoft/DirectX-Headers/${DIRECTX_HEADERS_VERSION}/")

foreach(_header IN LISTS DIRECTX_HEADERS_FILES)
    # 构建源文件的完整路径
    set(SOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${_header}")
    
    # 检查文件是否存在
    if(NOT EXISTS "${SOURCE_FILE}")
        message(STATUS "Downloading ${_header}...")
        
        # 构建下载URL
        set(DOWNLOAD_URL "${DIRECTX_HEADERS_BASE_URL}${_header}")
        
        # 下载文件
        file(DOWNLOAD 
            ${DOWNLOAD_URL} 
            ${SOURCE_FILE} 
            STATUS _download_status
            SHOW_PROGRESS
        )
        
        # 检查下载是否成功
        list(GET _download_status 0 _status_code)
        if("${_status_code}" EQUAL 0)
            message(STATUS "Downloaded ${_header} successfully.")
        else()
            message(FATAL_ERROR "Failed to download ${_header}. Status code: ${_status_code}")
        endif()
    else()
        message(STATUS "${_header} already exists. Skipping download.")
    endif()
endforeach()

set(TARGET_NAME DirectX-Headers)

add_library(${TARGET_NAME} INTERFACE)
target_sources(${TARGET_NAME} PRIVATE ${DIRECTX_HEADERS_FILES})
target_include_directories(${TARGET_NAME} INTERFACE include/directx)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER ThirdParty)