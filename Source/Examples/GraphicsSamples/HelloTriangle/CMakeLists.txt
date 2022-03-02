set(TARGET_NAME HelloTriangle)

add_executable(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE SketchApp.cpp)

# 私有链接库
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Source/Launcher)
target_link_libraries(${TARGET_NAME} PRIVATE Launcher)

target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Source/Sketch)
target_link_libraries(${TARGET_NAME} PRIVATE Sketch)

target_link_libraries(${TARGET_NAME} PRIVATE DirectX-Headers)

target_link_libraries(${TARGET_NAME} PRIVATE dxgi.lib d3d12.lib d3dcompiler.lib)

# 寻找fxc
get_filename_component(WINDOWS_KITS_DIR
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]" ABSOLUTE)
set(FXC ${WINDOWS_KITS_DIR}/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64/fxc.exe)

# 将shader(hlsl)文件加入VS工程
set(SHADER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Shaders.hlsl)
target_sources(${TARGET_NAME} PRIVATE ${SHADER_FILE})
set_property(SOURCE ${SHADER_FILE} PROPERTY VS_SETTINGS "ExcludedFromBuild=true")
source_group("Shader Files" FILES ${SHADER_FILE})

# shader文件编译所生成头文件的存放路径
set(GENERATED_SHADER_HEADER_DIR ${CMAKE_CURRENT_BINARY_DIR}/GeneratedShaderHeaders)
get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)
set(SHADER_HEADER_VS ${GENERATED_SHADER_HEADER_DIR}/${SHADER_NAME}VS.h)
set(SHADER_HEADER_PS ${GENERATED_SHADER_HEADER_DIR}/${SHADER_NAME}PS.h)

# shader头文件加入target，实现对该文件的依赖
target_sources(${TARGET_NAME} PRIVATE ${SHADER_HEADER_VS})
target_sources(${TARGET_NAME} PRIVATE ${SHADER_HEADER_PS})
target_include_directories(${TARGET_NAME} PRIVATE ${GENERATED_SHADER_HEADER_DIR})

# fxc编译命令
set(FXC_DEBUG ${FXC} /nologo /Vi /Od /Zi)
set(FXC_RELEASE ${FXC} /nologo /Vi)
set(FXC_VS_FLAG /T vs_5_0 /E VSMain)
set(FXC_PS_FLAG /T ps_5_0 /E PSMain)

add_custom_command(
    OUTPUT ${SHADER_HEADER_VS}
    COMMAND ${FXC_DEBUG} ${SHADER_FILE} ${FXC_VS_FLAG} /Fh ${SHADER_HEADER_VS}
    DEPENDS ${SHADER_FILE}
    VERBATIM
)

add_custom_command(
    OUTPUT ${SHADER_HEADER_PS}
    COMMAND ${FXC_DEBUG} ${SHADER_FILE} ${FXC_PS_FLAG} /Fh ${SHADER_HEADER_PS}
    DEPENDS ${SHADER_FILE}
    VERBATIM
)