#include "Launcher.h"

#include <stdexcept>
#include <windowsx.h>
#include <iostream>
#include "SketchBase.h"

static HWND SMainWindow = nullptr;
static sketch::SketchBase* SSketchInstance = nullptr;

namespace launcher
{

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static bool bSleeping = false;
    static bool bSizingOrMoving = false;

    switch (message)
    {
    case WM_SYSKEYDOWN:
    {
        // https://docs.microsoft.com/en-us/windows/win32/learnwin32/keyboard-input?redirectedfrom=MSDN
        // One flag that might be useful is bit 30, the "previous key state" flag, which is set to 1 for repeated key-down messages.
        if (wParam == VK_RETURN && !(lParam & (1 << 30)) &&
            SSketchInstance->GetConfig().WindowModeSwitch && SSketchInstance->GetFeature().Tearing)
        {
            ToggleFullscreen();
        }
    }
    break;

    case WM_SIZE:
    {
        switch (wParam)
        {
        case SIZE_MINIMIZED:
        {
            bSleeping = true;
            SSketchInstance->Pause();
        }
            break;

        case SIZE_MAXIMIZED:
        case SIZE_RESTORED:
            if (!bSizingOrMoving)
            {
                if (bSleeping)
                {
                    bSleeping = false;
                    SSketchInstance->Resume();
                }
                SSketchInstance->Resize(static_cast<int>(LOWORD(lParam)), static_cast<int>(HIWORD(lParam)));
            }
            break;

        default:
            break;
        }
    }
    return 0;

    case WM_ENTERSIZEMOVE:
    {
        bSizingOrMoving = true;
    }
    return 0;

    case WM_EXITSIZEMOVE:
    {
        bSizingOrMoving = false;

        RECT rc;
        GetClientRect(hWnd, &rc);
        SSketchInstance->Resize(static_cast<int>(rc.right - rc.left), static_cast<int>(rc.bottom - rc.top));
    }
    return 0;

    case WM_LBUTTONDOWN:
    {
        SSketchInstance->MouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), sketch::MouseButtonType::kLeft);
    }
    return 0;

    case WM_LBUTTONUP:
    {
        SSketchInstance->MouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), sketch::MouseButtonType::kLeft);
    }
    return 0;

    case WM_RBUTTONDOWN:
    {
        SSketchInstance->MouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), sketch::MouseButtonType::kRight);
    }
    return 0;

    case WM_RBUTTONUP:
    {
        SSketchInstance->MouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), sketch::MouseButtonType::kRight);
    }
    return 0;

    case WM_MOUSEMOVE:
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        if ((DWORD)wParam & MK_LBUTTON)
        {
            SSketchInstance->MouseDrag(x, y, sketch::MouseButtonType::kLeft);
        }
        else if ((DWORD)wParam & MK_RBUTTON)
        {
            SSketchInstance->MouseDrag(x, y, sketch::MouseButtonType::kRight);
        }
        else
        {
            SSketchInstance->MouseMove(x, y);
        }
    }
    return 0;

    case WM_DESTROY:
    {
        PostQuitMessage(0);
    }
    return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

static RECT GetFullscreenRect()
{
    SetWindowLong(SMainWindow, GWL_STYLE, WS_OVERLAPPED);

    DEVMODE devMode = {};
    devMode.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devMode);

    RECT fullscreenWindowRect = {
        devMode.dmPosition.x,
        devMode.dmPosition.y,
        devMode.dmPosition.x + static_cast<LONG>(devMode.dmPelsWidth),
        devMode.dmPosition.y + static_cast<LONG>(devMode.dmPelsHeight)
    };

    return fullscreenWindowRect;
}

static void RunInternal(sketch::SketchBase* sketchInstance, const std::string& sketchName, std::function<void(sketch::SketchBase::Config&)> configSetter)
{
    SSketchInstance = sketchInstance;

    if (configSetter)
    {
        sketchInstance->SetConfig(configSetter);
    }

    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIconW(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)COLOR_WINDOWFRAME;
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"LauncherClass";
    wcex.hIconSm = LoadIconW(hInstance, IDI_APPLICATION);

    RegisterClassExW(&wcex);

    int count = MultiByteToWideChar(CP_UTF8, 0, sketchName.c_str(), (int)sketchName.length(), nullptr, 0);
    std::wstring sketchNameWide(count, 0);
    MultiByteToWideChar(CP_UTF8, 0, sketchName.c_str(), (int)sketchName.length(), &sketchNameWide[0], count);

    RECT rc = { 
        static_cast<LONG>(sketchInstance->GetConfig().X), 
        static_cast<LONG>(sketchInstance->GetConfig().Y),
        static_cast<LONG>(sketchInstance->GetConfig().X + sketchInstance->GetConfig().Width), 
        static_cast<LONG>(sketchInstance->GetConfig().Y + sketchInstance->GetConfig().Height)
    };
    DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&rc, style, FALSE);

    SMainWindow = CreateWindowW(wcex.lpszClassName, sketchNameWide.c_str(), style, rc.left, rc.top,
        rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);

    int cmdShow = SW_SHOWDEFAULT;
    if (sketchInstance->GetConfig().Fullscreen)
    {
        SetWindowLong(SMainWindow, GWL_STYLE, WS_OVERLAPPED);
        RECT rect = GetFullscreenRect();
        SetWindowPos(SMainWindow, nullptr, rect.left, rect.top,
            rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE | SWP_NOSIZE);
        cmdShow = SW_MAXIMIZE;
    }

    sketchInstance->Init();

    ShowWindow(SMainWindow, cmdShow);
    UpdateWindow(SMainWindow);

    sketchInstance->Reset();
    MSG msg;
    do
    {
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        sketchInstance->Update();
        sketchInstance->Tick();
    } while (TRUE);

    sketchInstance->Quit();
}

void Run(std::shared_ptr<sketch::SketchBase> sketchInstance, const std::string& sketchName, std::function<void(sketch::SketchBase::Config&)> configSetter)
{
    try
    {
        RunInternal(sketchInstance.get(), sketchName, configSetter);
    }
    catch (const std::runtime_error& e)
    {
        std::string errorString(e.what());
        int count = MultiByteToWideChar(CP_UTF8, 0, errorString.c_str(), (int)errorString.length(), nullptr, 0);
        std::wstring errorStringWide(count, 0);
        MultiByteToWideChar(CP_UTF8, 0, errorString.c_str(), (int)errorString.length(), &errorStringWide[0], count);
        MessageBoxW(nullptr, errorStringWide.c_str(), L"Failed", MB_OK);
    }
}

HWND GetMainWindow()
{
    return SMainWindow;
}

void ToggleFullscreen()
{
    static bool fullscreen = SSketchInstance->GetConfig().Fullscreen;
    static RECT windowModeRect = {
        static_cast<LONG>(SSketchInstance->GetConfig().X),
        static_cast<LONG>(SSketchInstance->GetConfig().Y),
        static_cast<LONG>(SSketchInstance->GetConfig().X + SSketchInstance->GetConfig().Width),
        static_cast<LONG>(SSketchInstance->GetConfig().Y + SSketchInstance->GetConfig().Height)
    };

    fullscreen = !fullscreen;

    int cmdShow = SW_SHOWDEFAULT;
    RECT rect = windowModeRect;
    if (fullscreen)
    {
        GetWindowRect(SMainWindow, &windowModeRect);
        
        rect = GetFullscreenRect();
        cmdShow = SW_MAXIMIZE;

        SetWindowLong(SMainWindow, GWL_STYLE, WS_OVERLAPPED);
    }
    else
    {
        SetWindowLong(SMainWindow, GWL_STYLE, WS_OVERLAPPEDWINDOW);
    }

    SetWindowPos(SMainWindow, nullptr, rect.left, rect.top,
        rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE | SWP_FRAMECHANGED);
    ShowWindow(SMainWindow, cmdShow);
}

}; // namespace launcher