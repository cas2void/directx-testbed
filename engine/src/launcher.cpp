#include "launcher.h"

#include <stdexcept>
#include <windowsx.h>

namespace
{
RECT GetFullscreenRect()
{
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
class Launcher
{
public:
    static void Run(const std::string& sketch_name)
    {
        sketch_instance->Configure();

        SetProcessDPIAware();

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

        const auto& config = sketch_instance->GetConfig();
        const auto [screen_left, screen_top, screen_right, screen_bottom] = GetFullscreenRect();
        const auto fullscreen_width = screen_right - screen_left;
        const auto fullscreen_height = screen_bottom - screen_top;

        const auto valid_width = config.width > 0 ? config.width : fullscreen_width / 2;
        const auto valid_height = config.height > 0 ? config.height : fullscreen_height / 2;
        const auto valid_x = config.x >= 0 ? config.x : (fullscreen_width - valid_width) / 2;
        const auto valid_y = config.y >= 0 ? config.y : (fullscreen_height - valid_height) / 2;

        RECT valid_window_rect = {valid_x, valid_y, valid_x + valid_width, valid_y + valid_height};

        constexpr DWORD style = WS_OVERLAPPEDWINDOW;
        AdjustWindowRect(&valid_window_rect, style, FALSE);

        const auto adjusted_width = valid_window_rect.right - valid_window_rect.left;
        const auto adjusted_height = valid_window_rect.bottom - valid_window_rect.top;

        const auto sketch_name_wide = ConvertNarrowStringToWideString(sketch_name);
        main_window = CreateWindowW(window_class.lpszClassName,
                                     sketch_name_wide.c_str(),
                                     style,
                                     valid_x,
                                     valid_y,
                                     adjusted_width,
                                     adjusted_height,
                                     nullptr,
                                     nullptr,
                                     hInstance,
                                     nullptr);

        int cmd_show = SW_SHOWDEFAULT;
        if (config.fullscreen)
        {
            SetWindowLong(main_window, GWL_STYLE, WS_OVERLAPPED);
            const auto [left, top, right, bottom] = GetFullscreenRect();
            SetWindowPos(main_window,
                         nullptr,
                         left,
                         top,
                         right - left,
                         bottom - top,
                         SWP_NOACTIVATE | SWP_NOSIZE);
            cmd_show = SW_MAXIMIZE;
        }

        sketch_instance->Init();

        ShowWindow(main_window, cmd_show);
        UpdateWindow(main_window);

        sketch_instance->Reset();
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
            sketch_instance->Tick();
        }
        while (TRUE);

        sketch_instance->Quit();
    }

    static LRESULT CALLBACK WndProc(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param)
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
                        sketch_instance->Pause();
                    }
                    break;

                case SIZE_MAXIMIZED:
                case SIZE_RESTORED:
                    if (!sizing_or_moving)
                    {
                        if (sleeping)
                        {
                            sleeping = false;
                            sketch_instance->Resume();
                        }

                        RECT rc;
                        GetClientRect(wnd, &rc);
                        sketch_instance->Resize(static_cast<int>(rc.right - rc.left),
                                                 static_cast<int>(rc.bottom - rc.top));
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
                sketch_instance->Resize(static_cast<int>(rc.right - rc.left),
                                         static_cast<int>(rc.bottom - rc.top));
            }
            break;

        case WM_LBUTTONDOWN:
            {
                sketch_instance->MouseDown(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param), MouseButtonType::kLeft);
            }
            break;

        case WM_LBUTTONUP:
            {
                sketch_instance->MouseUp(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param), MouseButtonType::kLeft);
            }
            break;

        case WM_RBUTTONDOWN:
            {
                sketch_instance->MouseDown(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param), MouseButtonType::kRight);
            }
            break;

        case WM_RBUTTONUP:
            {
                sketch_instance->MouseUp(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param), MouseButtonType::kRight);
            }
            break;

        case WM_MOUSEMOVE:
            {
                const auto x = GET_X_LPARAM(l_param);
                const auto y = GET_Y_LPARAM(l_param);
                if (static_cast<DWORD>(w_param) & MK_LBUTTON)
                {
                    sketch_instance->MouseDrag(x, y, MouseButtonType::kLeft);
                }
                else if (static_cast<DWORD>(w_param) & MK_RBUTTON)
                {
                    sketch_instance->MouseDrag(x, y, MouseButtonType::kRight);
                }
                else
                {
                    sketch_instance->MouseMove(x, y);
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

    static std::shared_ptr<SketchBase> sketch_instance;
    static HWND main_window;
};

std::shared_ptr<SketchBase> Launcher::sketch_instance;
HWND Launcher::main_window;

void ToggleFullscreen()
{
    static auto fullscreen = Launcher::sketch_instance->GetConfig().fullscreen;

    fullscreen = !fullscreen;

    int cmd_show = SW_SHOWDEFAULT;
    RECT current_rect;
    GetWindowRect(Launcher::main_window, &current_rect);

    if (fullscreen)
    {
        current_rect = GetFullscreenRect();
        cmd_show = SW_MAXIMIZE;

        SetWindowLong(Launcher::main_window, GWL_STYLE, WS_OVERLAPPED);
    }
    else
    {
        SetWindowLong(Launcher::main_window, GWL_STYLE, WS_OVERLAPPEDWINDOW);
    }

    SetWindowPos(Launcher::main_window,
                 nullptr,
                 current_rect.left,
                 current_rect.top,
                 current_rect.right - current_rect.left,
                 current_rect.bottom - current_rect.top,
                 SWP_NOACTIVATE | SWP_FRAMECHANGED);
    ShowWindow(Launcher::main_window, cmd_show);
}

HWND GetMainWindow()
{
    return Launcher::main_window;
}

// void Launcher::Launch(const std::shared_ptr<SketchBase>& sketch_instance, const std::string& sketch_name)
void Launch(const std::shared_ptr<SketchBase>& sketch_instance, const std::string& sketch_name)
{
    Launcher::sketch_instance = sketch_instance;

    try
    {
        Launcher::Run(sketch_name);
    }
    catch (const std::runtime_error& e)
    {
        const std::string error_string(e.what());
        const auto error_string_wide = ConvertNarrowStringToWideString(error_string);
        MessageBoxW(nullptr, error_string_wide.c_str(), L"Failed", MB_OK);
    }
}
}; // namespace launcher
