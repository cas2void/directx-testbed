#pragma once

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

#include <string>
#include <memory>

#include "sketch-base.h"

namespace engine
{
class Launcher
{
public:
    static void Launch(const std::shared_ptr<SketchBase>& sketch_instance, const std::string& sketch_name);
    static void ToggleFullscreen();

private:
    static void Run(const std::string& sketch_name);
    static LRESULT CALLBACK WndProc(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param);
    
    static std::shared_ptr<SketchBase> sketch_instance_;
    static HWND main_window_;
};
}

#define LAUNCH_SKETCH(SketchType) \
int APIENTRY wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int) \
{ \
    auto sketch_instance = std::make_shared<SketchType>(); \
    engine::Launcher::Launch(sketch_instance, #SketchType);\
    return 0; \
}
