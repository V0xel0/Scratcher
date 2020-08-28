#include "window.h"

LRESULT CALLBACK
mainWindowCallback(
	HWND window,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	LRESULT result = 0;

	switch (message)
	{
	case WM_SIZE: //chng window size
	{
		int width = LOWORD(lParam); // Macro to get the low-order word.
		int height = HIWORD(lParam); // Macro to get the high-order word.
			//TODO:: Need to inform render system about size change
	}
	break;

	case WM_ACTIVATEAPP: // when window clicked --> gets active
	{
		OutputDebugStringA("WM_ACTIVATEAPP\n");
	}
	break;

	case WM_DESTROY: //when window get deleted
	{
		OutputDebugStringA("Killed\n");
		PostQuitMessage(0);
	}
	break;

	case WM_PAINT:
	{

	}
	break;

	case WM_MENUCHAR:
	{
		result = MAKELRESULT(0, MNC_CLOSE);
	}
	break;

	default:
	{
		//            OutputDebugStringA("default\n");
		result = DefWindowProc(window, message, wParam, lParam); //other msgs get handled "default way" by this func
	}
	break;
	}

	return (result);
}

HWND CreateMainWindow(const int w, const int h, const char* name)
{
	HINSTANCE instance = nullptr;
	HWND mainWindow = nullptr;

	instance = GetModuleHandle(nullptr); //null retrieves .exe handle
	WNDCLASSEXA windowClass = {};

	windowClass.cbSize = sizeof(WNDCLASSEXA);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = mainWindowCallback;
	windowClass.hInstance = instance;
	//windowClass.hIcon = LoadIcon(instance, "IDI_WINLOGO");
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 6);
	windowClass.lpszClassName = name;
	//windowClass.hIconSm = LoadIcon(windowClass.hInstance, "IDI_ICON");

	const s32 error = RegisterClassExA(&windowClass);
	assert( error != 0 && "Class registration failed, error code:" && GetLastError());

	RECT rc = { 0, 0, static_cast<LONG>(w), static_cast<LONG>(h) };
	AdjustWindowRectEx(&rc, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW);
	const int winStyle = WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_OVERLAPPED | WS_SYSMENU;

	mainWindow = CreateWindowExA(
		WS_EX_APPWINDOW,
		name,
		name,
		winStyle,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		instance,
		nullptr);

	assert(mainWindow && "Window creation failed, error code:" && GetLastError());

	ShowWindow(mainWindow, SW_SHOW);
	SetForegroundWindow(mainWindow);
	SetFocus(mainWindow);

	return mainWindow;
}