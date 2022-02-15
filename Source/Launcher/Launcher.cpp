#include "Launcher.h"

static HWND SMainWindow = nullptr;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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

void LauncherMain(void)
{
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

	RECT rc = {0, 0, (LONG)960, (LONG)540};
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	SMainWindow = CreateWindowW(wcex.lpszClassName, L"Demo",  WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);

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
		}
	} while (TRUE);
}