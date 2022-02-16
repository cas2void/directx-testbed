#pragma once

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

#include <memory>
#include <string>
#include <functional>

namespace sketch
{

struct SketchConfig;
class SketchBase;

}; // namespace sketch

namespace launcher
{

void Run(std::shared_ptr<sketch::SketchBase> sketchInstance, const std::string& sketchName,
    std::function<void(sketch::SketchConfig&)> configurator = std::function<void(sketch::SketchConfig&)>());

}; // namespace launcher

#if NDEBUG
#pragma comment(linker, "/subsystem:windows")
#define CREATE_SKETCH(sketch) \
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) \
{ \
    UNREFERENCED_PARAMETER(hInstance); \
    UNREFERENCED_PARAMETER(hPrevInstance); \
    UNREFERENCED_PARAMETER(lpCmdLine); \
    UNREFERENCED_PARAMETER(nCmdShow); \
    launcher::Run(std::make_shared<SketchType>(), #SketchType, __VA_ARGS__); \
    return 0; \
}
#else
#pragma comment(linker, "/subsystem:console")
#define CREATE_SKETCH(SketchType, ...) \
int main(int argc, char* argv[]) \
{ \
    (void)argc; \
    (void)argv; \
    launcher::Run(std::make_shared<SketchType>(), #SketchType, __VA_ARGS__); \
    return 0; \
}
#endif