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

# Shaders
set(GENERATED_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/GeneratedHeaders)
file(MAKE_DIRECTORY ${GENERATED_DIRECTORY})
set(FXC_RELEASE_FLAGS "/nologo" "/Vi")
set(FXC_DEBUG_FLAGS ${FXC_RELEASE_FLAGS} "/Zi" "/Od")
add_custom_command(
	TARGET ${TARGET_NAME} 
	PRE_BUILD
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Shaders.hlsl
 	COMMAND if $<CONFIG> equ Debug (fxc ${FXC_DEBUG_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Shaders.hlsl /E VSMain /T vs_5_0 /Fh ${GENERATED_DIRECTORY}/ShaderVS.h)
	COMMAND if $<CONFIG> neq Debug (fxc ${FXC_RELEASE_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Shaders.hlsl /E VSMain /T vs_5_0 /Fh ${GENERATED_DIRECTORY}/ShaderVS.h)
	COMMAND if $<CONFIG> equ Debug (fxc ${FXC_DEBUG_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Shaders.hlsl /E PSMain /T ps_5_0 /Fh ${GENERATED_DIRECTORY}/ShaderPS.h)
	COMMAND if $<CONFIG> neq Debug (fxc ${FXC_RELEASE_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Shaders.hlsl /E PSMain /T ps_5_0 /Fh ${GENERATED_DIRECTORY}/ShaderPS.h)
	VERBATIM)
set(SHADER_FILES Shaders/Shaders.hlsl)
target_sources(${TARGET_NAME} PRIVATE ${SHADER_FILES})
set_property(SOURCE ${SHADER_FILES} PROPERTY VS_SETTINGS "ExcludedFromBuild=true")
source_group("Shader Files" FILES ${SHADER_FILES})

target_include_directories(${TARGET_NAME} PRIVATE ${GENERATED_DIRECTORY})