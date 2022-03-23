#pragma once

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

#include <string>
#include <functional>

#include "SketchBase.h"

namespace launcher
{

void Run(sketch::SketchBase* sketchInstance, const std::string& sketchName,
    std::function<void(sketch::SketchBase::Config&)> configSetter = std::function<void(sketch::SketchBase::Config&)>());

HWND GetMainWindow();

void ToggleFullscreen();

}; // namespace launcher

#if NDEBUG
#pragma comment(linker, "/subsystem:windows")
#define CREATE_SKETCH(SketchType, ...) \
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) \
{ \
    UNREFERENCED_PARAMETER(hInstance); \
    UNREFERENCED_PARAMETER(hPrevInstance); \
    UNREFERENCED_PARAMETER(lpCmdLine); \
    UNREFERENCED_PARAMETER(nCmdShow); \
    SketchType sketchInstance; \
    launcher::Run(&sketchInstance, #SketchType, __VA_ARGS__); \
    return 0; \
}
#else
#pragma comment(linker, "/subsystem:console")
#define CREATE_SKETCH(SketchType, ...) \
int main(int argc, char* argv[]) \
{ \
    (void)argc; \
    (void)argv; \
    SketchType sketchInstance; \
    launcher::Run(&sketchInstance, #SketchType, __VA_ARGS__); \
    return 0; \
}
#endif