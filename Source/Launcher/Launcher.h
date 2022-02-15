#pragma once

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

void LauncherMain();

#if NDEBUG
#pragma comment(linker, "/subsystem:windows")
#define CREATE_SKETCH(sketch) \
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) \
{ \
    UNREFERENCED_PARAMETER(hInstance); \
    UNREFERENCED_PARAMETER(hPrevInstance); \
    UNREFERENCED_PARAMETER(lpCmdLine); \
    UNREFERENCED_PARAMETER(nCmdShow); \
    LauncherMain(); \
    return 0; \
}
#else
#pragma comment(linker, "/subsystem:console")
#define CREATE_SKETCH(sketch) \
int main(int argc, char* argv[]) \
{ \
    (void)argc; \
    (void)argv; \
    LauncherMain(); \
    return 0; \
}
#endif