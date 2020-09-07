#pragma once
#include "Win32WindowsFiles.h"

namespace Win32
{
	struct ScreenBuffer
	{
		BITMAPINFO info;
		void *memory;
		s32 width;
		s32 height;
		s32 pitch;
	};

	struct WindowDimensions
	{
		s32 width;
		s32 height;
	};

	// One 4 bytes per pixel buffer with one not shared device context is assumed, so for API simplicity, 
	// those two are internal globals and they are never exposed to application layer directly
	constexpr global_variable s32 bytesPerPixel = 4;
	global_variable ScreenBuffer internalBuffer = {};

	//TODO: Change allocation model to not allocate and just get memory from outside or if not then consider wrapping to RAII
	// Used to create a new Win32 Screen Buffer with 4 bytes pixels with BGRA memory order
	void ResizeInternalBuffer(ScreenBuffer *buffer, const s32 w, const s32 h)
	{
		assert(buffer != nullptr);
		if(buffer->memory)
		{
			VirtualFree(buffer->memory, 0, MEM_RELEASE);
		}
		buffer->width = w;
		buffer->height = h;
		const s32 pixelSize = 4;
		// Set header info for bitmap
		buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
		buffer->info.bmiHeader.biWidth = w;
		buffer->info.bmiHeader.biHeight = -h;
		buffer->info.bmiHeader.biPlanes = 1;
		buffer->info.bmiHeader.biBitCount = 32;
		buffer->info.bmiHeader.biCompression = BI_RGB;

		buffer->pitch = AlignAddress16(buffer->width*bytesPerPixel);
		u32 bitmapMemorySize = buffer->pitch*buffer->height;
		buffer->memory = VirtualAlloc(nullptr, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

		buffer->pitch = w*pixelSize;
	}

	// Used for updating window contents when WM_PAINT msg from windows appears or when platform layers wants to update
	void UpdateWindow(HDC deviceCtx, s32 width, s32 height, ScreenBuffer* buffer)
	{
		// BitBlt might be faster
		StretchDIBits(
			deviceCtx, 
			0,0,width,height,
			0,0,buffer->width,buffer->height, 
			buffer->memory, &buffer->info, 
			DIB_RGB_COLORS, SRCCOPY);
	}

	WindowDimensions GetWindowClientDimensions(HWND window)
	{
		WindowDimensions out;
		RECT rect;
		GetClientRect(window, &rect);
		out.width = rect.right - rect.left;
		out.height = rect.bottom - rect.top;
		return out;
	}

	// Windows callback functions for window messages processing. It is being called by "DispatchMessage" or directly by Windows
	LRESULT CALLBACK mainWindowCallback( HWND window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		LRESULT result = 0;

		switch (message)
		{
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

				WindowDimensions dims = GetWindowClientDimensions(window);
				UpdateWindow(deviceCtx, dims.width, dims.height, &internalBuffer);
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

	// Creates window for current process, cause of CS_OWNDC device context is assumed to not be shared with anyone
	HWND CreateMainWindow(const s32 w, const s32 h, const char* name)
	{
		HINSTANCE instance = nullptr;
		HWND mainWindow = nullptr;

		instance = GetModuleHandle(nullptr);
		WNDCLASSEXA windowClass = {};

		windowClass.cbSize = sizeof(WNDCLASSEXA);
		windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
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
}