#include "launcher.h"

#include <stdexcept>
#include <windowsx.h>

namespace
{
RECT GetFullscreenRect(const HWND window)
{
    SetWindowLong(window, GWL_STYLE, WS_OVERLAPPED);

    DEVMODE dev_mode = {};
    dev_mode.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &dev_mode);

    const RECT fullscreen_window_rect = {
        dev_mode.dmPosition.x,
        dev_mode.dmPosition.y,
        dev_mode.dmPosition.x + static_cast<LONG>(dev_mode.dmPelsWidth),
        dev_mode.dmPosition.y + static_cast<LONG>(dev_mode.dmPelsHeight)
    };

    return fullscreen_window_rect;
}

std::wstring ConvertNarrowStringToWideString(const std::string& narrow_string)
{
    const auto count = MultiByteToWideChar(CP_UTF8,
                                           0,
                                           narrow_string.c_str(),
                                           static_cast<int>(narrow_string.length()),
                                           nullptr,
                                           0);
    std::wstring wide_string(count, 0);
    MultiByteToWideChar(CP_UTF8,
                        0,
                        narrow_string.c_str(),
                        static_cast<int>(narrow_string.length()),
                        wide_string.data(),
                        count);
    return wide_string;
}
}

namespace engine
{
std::shared_ptr<SketchBase> Launcher::sketch_instance_;
HWND Launcher::main_window_;

void Launcher::ToggleFullscreen()
{
    static auto fullscreen = sketch_instance_->GetConfig().fullscreen;
    static RECT window_mode_rect = {
        static_cast<LONG>(sketch_instance_->GetConfig().x),
        static_cast<LONG>(sketch_instance_->GetConfig().y),
        static_cast<LONG>(sketch_instance_->GetConfig().x + sketch_instance_->GetConfig().width),
        static_cast<LONG>(sketch_instance_->GetConfig().y + sketch_instance_->GetConfig().height)
    };

    fullscreen = !fullscreen;

    int cmd_show = SW_SHOWDEFAULT;
    auto current_rect = window_mode_rect;
    if (fullscreen)
    {
        GetWindowRect(main_window_, &window_mode_rect);

        current_rect = GetFullscreenRect(main_window_);
        cmd_show = SW_MAXIMIZE;

        SetWindowLong(main_window_, GWL_STYLE, WS_OVERLAPPED);
    }
    else
    {
        SetWindowLong(main_window_, GWL_STYLE, WS_OVERLAPPEDWINDOW);
    }

    SetWindowPos(main_window_,
                 nullptr,
                 current_rect.left,
                 current_rect.top,
                 current_rect.right - current_rect.left,
                 current_rect.bottom - current_rect.top,
                 SWP_NOACTIVATE | SWP_FRAMECHANGED);
    ShowWindow(main_window_, cmd_show);
}

void Launcher::Launch(const std::shared_ptr<SketchBase>& sketch_instance, const std::string& sketch_name)
{
    sketch_instance_ = sketch_instance;

    try
    {
        Run(sketch_name);
    }
    catch (const std::runtime_error& e)
    {
        const std::string error_string(e.what());
        const auto error_string_wide = ConvertNarrowStringToWideString(error_string);
        MessageBoxW(nullptr, error_string_wide.c_str(), L"Failed", MB_OK);
    }
}

void Launcher::Run(const std::string& sketch_name)
{
    sketch_instance_->Configure();

    const HINSTANCE hInstance = GetModuleHandleW(nullptr);

    WNDCLASSEXW window_class;
    window_class.cbSize = sizeof(WNDCLASSEXW);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = WndProc;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = hInstance;
    window_class.hIcon = LoadIconW(hInstance, IDI_APPLICATION);
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOWFRAME);
    window_class.lpszMenuName = nullptr;
    window_class.lpszClassName = L"LauncherClass";
    window_class.hIconSm = LoadIconW(hInstance, IDI_APPLICATION);

    RegisterClassExW(&window_class);

    RECT rc = {
        static_cast<LONG>(sketch_instance_->GetConfig().x),
        static_cast<LONG>(sketch_instance_->GetConfig().y),
        static_cast<LONG>(sketch_instance_->GetConfig().x + sketch_instance_->GetConfig().width),
        static_cast<LONG>(sketch_instance_->GetConfig().y + sketch_instance_->GetConfig().height)
    };
    constexpr DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&rc, style, FALSE);

    const auto sketch_name_wide = ConvertNarrowStringToWideString(sketch_name);
    main_window_ = CreateWindowW(window_class.lpszClassName,
                                 sketch_name_wide.c_str(),
                                 style,
                                 rc.left,
                                 rc.top,
                                 rc.right - rc.left,
                                 rc.bottom - rc.top,
                                 nullptr,
                                 nullptr,
                                 hInstance,
                                 nullptr);

    int cmd_show = SW_SHOWDEFAULT;
    if (sketch_instance_->GetConfig().fullscreen)
    {
        SetWindowLong(main_window_, GWL_STYLE, WS_OVERLAPPED);
        const auto [left, top, right, bottom] = GetFullscreenRect(main_window_);
        SetWindowPos(main_window_,
                     nullptr,
                     left,
                     top,
                     right - left,
                     bottom - top,
                     SWP_NOACTIVATE | SWP_NOSIZE);
        cmd_show = SW_MAXIMIZE;
    }

    sketch_instance_->Init();

    ShowWindow(main_window_, cmd_show);
    UpdateWindow(main_window_);

    sketch_instance_->Reset();
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
        sketch_instance_->Tick();
    }
    while (TRUE);

    sketch_instance_->Quit();
}

LRESULT Launcher::WndProc(const HWND wnd, const UINT message, const WPARAM w_param, const LPARAM l_param)
{
    static auto sleeping = false;
    static auto sizing_or_moving = false;

    switch (message)
    {
    // case WM_SYSKEYDOWN:
    //     {
    //         // https://docs.microsoft.com/en-us/windows/win32/learnwin32/keyboard-input?redirectedfrom=MSDN
    //         // One flag that might be useful is bit 30, the "previous key state" flag, which is set to 1 for repeated key-down messages.
    //         if (wParam == VK_RETURN && !(lParam & (1 << 30)) &&
    //             g_sketch_instance->GetConfig().window_mode_switch && g_sketch_instance->GetFeature().tearing)
    //         {
    //             engine::ToggleFullscreen();
    //         }
    //     }
    //     break;

    case WM_SIZE:
        {
            switch (w_param)
            {
            case SIZE_MINIMIZED:
                {
                    sleeping = true;
                    sketch_instance_->Pause();
                }
                break;

            case SIZE_MAXIMIZED:
            case SIZE_RESTORED:
                if (!sizing_or_moving)
                {
                    if (sleeping)
                    {
                        sleeping = false;
                        sketch_instance_->Resume();
                    }
                    sketch_instance_->Resize(LOWORD(l_param), HIWORD(l_param));
                }
                break;

            default:
                break;
            }
        }
        break;

    case WM_ENTERSIZEMOVE:
        {
            sizing_or_moving = true;
        }
        break;

    case WM_EXITSIZEMOVE:
        {
            sizing_or_moving = false;

            RECT rc;
            GetClientRect(wnd, &rc);
            sketch_instance_->Resize(static_cast<int>(rc.right - rc.left),
                                     static_cast<int>(rc.bottom - rc.top));
        }
        break;

    case WM_LBUTTONDOWN:
        {
            sketch_instance_->MouseDown(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param), engine::MouseButtonType::kLeft);
        }
        break;

    case WM_LBUTTONUP:
        {
            sketch_instance_->MouseUp(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param), engine::MouseButtonType::kLeft);
        }
        break;

    case WM_RBUTTONDOWN:
        {
            sketch_instance_->MouseDown(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param), engine::MouseButtonType::kRight);
        }
        break;

    case WM_RBUTTONUP:
        {
            sketch_instance_->MouseUp(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param), engine::MouseButtonType::kRight);
        }
        break;

    case WM_MOUSEMOVE:
        {
            const auto x = GET_X_LPARAM(l_param);
            const auto y = GET_Y_LPARAM(l_param);
            if (static_cast<DWORD>(w_param) & MK_LBUTTON)
            {
                sketch_instance_->MouseDrag(x, y, engine::MouseButtonType::kLeft);
            }
            else if (static_cast<DWORD>(w_param) & MK_RBUTTON)
            {
                sketch_instance_->MouseDrag(x, y, engine::MouseButtonType::kRight);
            }
            else
            {
                sketch_instance_->MouseMove(x, y);
            }
        }
        break;

    case WM_DESTROY:
        {
            PostQuitMessage(0);
        }
        break;
    default:
        return DefWindowProc(wnd, message, w_param, l_param);
    }

    return 0;
}
}; // namespace launcher
