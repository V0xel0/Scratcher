#include "window.h"

global_variable BITMAPINFO bitmapInfo;
global_variable void *bitmapMemory;
global_variable s32 bitmapWidth;
global_variable s32 bitmapHeight;
global_variable s32 bytesPerPixel = 4;

namespace Win32
{

// Used in WM_SIZE to create/recreate (in case of resizing) a new pixel buffer that we can write to
internal void ResizeDIBSection(const s32 w, const s32 h)
{
	if(bitmapMemory)
    {
        VirtualFree(bitmapMemory, 0, MEM_RELEASE);
    }
	bitmapWidth = w;
    bitmapHeight = h;
	// Set header info for bitmap
    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = w;
    bitmapInfo.bmiHeader.biHeight = -h;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
	// For now, allocation is inside meh...
	u32 bitmapMemorySize = (bitmapWidth*bitmapHeight)*bytesPerPixel;
    bitmapMemory = VirtualAlloc(nullptr, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	// TEST-ONLY and assumption that pitch is gonna align with pixels
	s32 pitch = w*bytesPerPixel;
	u32 *pixel = (u32*)bitmapMemory; 

	for (size_t y = 0; y < h; y++)
	{
		for (size_t x = 0; x < w; x++)
		{
			u8 b = (u8)x;
			u8 g = (u8)y;
			u8 r = 255;

			*pixel++ = ((r << 16) |(g << 8) | b);
		}
	}
}

// Used for updating window contents when WM_PAINT msg from windows appears
internal void UpdateWindow(HDC deviceCtx, RECT *clientRect)
{
	s32 width = clientRect->right - clientRect->left;
	s32 height = clientRect->bottom - clientRect->top;
	// BitBlt might be faster
	StretchDIBits(
		deviceCtx, 
		0,0,width,height,
		0,0,width,height, 
		bitmapMemory, &bitmapInfo, 
		DIB_RGB_COLORS, SRCCOPY);
}

}

// Windows callback functions for window messages processing. It is being called by "DispatchMessage" or directly by windows
LRESULT CALLBACK mainWindowCallback( HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	switch (message)
	{
	case WM_SIZE:
	{
		// For now, whenever the size change, we create new buffer and free old one
		RECT clientRect = {};
		GetClientRect(window, &clientRect);
		const s32 width = clientRect.right - clientRect.left;
		const s32 height = clientRect.bottom - clientRect.top;
		Win32::ResizeDIBSection(width, height);
	}
	break;

	case WM_ACTIVATEAPP:
	{
	}
	break;

	case WM_DESTROY:
	{
		OutputDebugStringA("Window Destroyed\n");
		PostQuitMessage(0);
	}
	break;

	case WM_PAINT:
	{
		// When WM_PAINT from windows occur, we get the area to repaint and update it
		PAINTSTRUCT Paint;
		HDC deviceCtx = BeginPaint(window, &Paint);
		s32 x = Paint.rcPaint.left;
		s32 y = Paint.rcPaint.top;
		s32 w = Paint.rcPaint.right - Paint.rcPaint.left;
		s32 h = Paint.rcPaint.bottom - Paint.rcPaint.top;

		RECT clientRect;
		GetClientRect(window, &clientRect);
		Win32::UpdateWindow(deviceCtx, &clientRect);
		EndPaint(window, &Paint);
	}
	break;

	case WM_MENUCHAR:
	{
		result = MAKELRESULT(0, MNC_CLOSE);
	}
	break;

	default:
	{
		result = DefWindowProc(window, message, wParam, lParam);
	}
	break;
	}

	return (result);
}

// Creates window for current process
HWND CreateMainWindow(const s32 w, const s32 h, const char* name)
{
	HINSTANCE instance = nullptr;
	HWND mainWindow = nullptr;

	instance = GetModuleHandle(nullptr);
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
	assert( error != 0 && "Class registration failed");

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

	assert(mainWindow != nullptr && "Window creation failed");

	ShowWindow(mainWindow, SW_SHOW);
	SetForegroundWindow(mainWindow);
	SetFocus(mainWindow);

	return mainWindow;
}