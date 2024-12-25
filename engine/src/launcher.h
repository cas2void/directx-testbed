#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>
#include <memory>

#include "sketch-base.h"

namespace engine
{
void ToggleFullscreen();
HWND GetMainWindow();

void Launch(const std::shared_ptr<SketchBase>& sketch_instance, const std::string& sketch_name);
}

#define LAUNCH_SKETCH(SketchType) \
int APIENTRY wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int) \
{ \
    auto sketch_instance = std::make_shared<SketchType>(); \
    engine::Launch(sketch_instance, #SketchType);\
    return 0; \
}
