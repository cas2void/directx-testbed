#include "Launcher.h"

#include <stdexcept>

#include "SketchBase.h"

static HWND SMainWindow = nullptr;

namespace launcher
{

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

static void RunInternal(std::shared_ptr<sketch::SketchBase> sketchInstance, const std::string& sketchName, std::function<void(sketch::SketchBase::Config&)> configurator)
{
	if (configurator)
	{
		sketchInstance->Configurate(configurator);
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

	RECT rc = { 0, 0, (LONG)sketchInstance->GetConfig().width, (LONG)sketchInstance->GetConfig().height };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	SMainWindow = CreateWindowW(wcex.lpszClassName, sketchNameWide.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);

	sketchInstance->Init();

	ShowWindow(SMainWindow, SW_SHOWDEFAULT);
	UpdateWindow(SMainWindow);

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

			sketchInstance->Update();
		}
	} while (TRUE);

	sketchInstance->Quit();
}

void Run(std::shared_ptr<sketch::SketchBase> sketchInstance, const std::string& sketchName, std::function<void(sketch::SketchBase::Config&)> configurator)
{
	try
	{
		RunInternal(sketchInstance, sketchName, configurator);
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

}; // namespace launcher