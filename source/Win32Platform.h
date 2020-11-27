#pragma once
#include "Win32WindowsFiles.h"
#include <Xinput.h>
#include <xaudio2.h>

// Win32 Platform layer implementations, intended to be used with "WINAPI WinMain" only!
// In order to provide distinction from Microsoft's WinApi functions "Win32" namespace is
// provided for all custom platform layer functions and structures
//! DO NOT INCLUDE IT ENYWHERE ELSE THAN IN WIN32 ENTRY POINT COMPILATION UNIT FILE!
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

	struct MouseDataRaw
	{
		s32 x;
		s32 y;
		s32 lastDx;
		s32 lastDy;
	};

	//====================================INTERNAL GLOBALS===============================================================================
	// Internal globals are never exposed to application layer directly, they are mostly data that
	// needs to be shared with window CALLBACK function in Windows

	global_variable MouseDataRaw mouseData;
	global_variable LARGE_INTEGER clockFrequency;

	// One 4 bytes per pixel buffer with one not shared device context is assumed
	constexpr global_variable s32 bytesPerPixel = 4;
	global_variable ScreenBuffer internalBuffer = {};

	//=================================================================================================================================

	//TODO: Change allocation model to not allocate and just get memory from outside or if not then consider wrapping to RAII
	// Used to create a new Win32 Screen Buffer with 4 bytes pixels with BGRA memory order
	internal void ResizeInternalBuffer(ScreenBuffer *buffer, const s32 w, const s32 h)
	{
		assert(buffer != nullptr);
		if (buffer->memory)
		{
			VirtualFree(buffer->memory, 0, MEM_RELEASE);
		}
		buffer->width = w;
		buffer->height = h;
		// Set header info for bitmap
		buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
		buffer->info.bmiHeader.biWidth = w;
		buffer->info.bmiHeader.biHeight = -h;
		buffer->info.bmiHeader.biPlanes = 1;
		buffer->info.bmiHeader.biBitCount = 32;
		buffer->info.bmiHeader.biCompression = BI_RGB;

		buffer->pitch = AlignAddress16(buffer->width * bytesPerPixel);
		u32 bitmapMemorySize = buffer->pitch * buffer->height;
		buffer->memory = VirtualAlloc(nullptr, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	}

	// Used for getting dimensions of client area for passed window
	internal WindowDimensions GetWindowClientDimensions(HWND window)
	{
		WindowDimensions out;
		RECT rect;
		GetClientRect(window, &rect);
		out.width = rect.right - rect.left;
		out.height = rect.bottom - rect.top;
		return out;
	}

	// Used for updating window contents when WM_PAINT msg from windows appears or when platform layer wants to update
	// It copies the color data from buffer rectangle to client area of a window, it also stretches the buffer to fit the client area
	internal void UpdateWindow(HDC deviceCtx, HWND window, ScreenBuffer *buffer)
	{
		assert(buffer != nullptr);
		Win32::WindowDimensions dims = Win32::GetWindowClientDimensions(window);
		//TODO: BitBlt might be faster
		StretchDIBits(
			deviceCtx,
			0, 0, dims.width, dims.height,
			0, 0, buffer->width, buffer->height,
			buffer->memory, &buffer->info,
			DIB_RGB_COLORS, SRCCOPY);
	}

	// Windows callback functions for window messages processing. It is being called by "DispatchMessage" or directly by Windows
	LRESULT CALLBACK mainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		LRESULT result = 0;

		switch (message)
		{
		// Used for obtaining relative mouse movement without acceleration
		case WM_INPUT:
		{
			u32 size = {};
			//TODO: This should come from external (and fast! - no dynamic allocation) memory source rather than guess
			constexpr u32 guessSize = 48; // seems like this is size for mouse
			u8 data[guessSize];
			RAWINPUT *raw = reinterpret_cast<RAWINPUT *>(data);

			// Cold call to get required size of the input data
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, data, &size, sizeof(RAWINPUTHEADER));

			//TODO: Remove it after proper memory handling for input data
			if (size > guessSize)
			{
				MessageBoxA(NULL, "Too small raw input data guessed!", "error", 0);
				break;
			}
			u32 copied = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, data, &size, sizeof(RAWINPUTHEADER));
			if (copied != size)
			{
				//TODO: Fail handling from GetRawInputData()
				MessageBoxA(NULL, "Incorrect raw input data size!", "error", 0);
				break;
			}
			// Only care about mouse delta
			if (raw->header.dwType == RIM_TYPEMOUSE &&
				(raw->data.mouse.lLastX != 0 || raw->data.mouse.lLastY != 0))
			{
				mouseData.lastDx = raw->data.mouse.lLastX;
				mouseData.lastDy = raw->data.mouse.lLastY;
			}
		}
		break;

		// Only used to get x,y coordinates of cursor in client area of the window
		case WM_MOUSEMOVE:
		{
			mouseData.x = LOWORD(lParam);
			mouseData.y = HIWORD(lParam);
		}
		break;

		// LMB RMB MMB down messages
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		{
		}
		break;

		// LMB RMB MMB up messages
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		{
		}
		break;

		// LMB RMB MMB double click messages
		case WM_LBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		{
		}
		break;

		// Keyboard input messages
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			WPARAM vkCode = wParam;
			b32 wasDown = TestBit(lParam, 30) != 0;
			b32 isDown = TestBit(lParam, 31) == 0;

			if (isDown != wasDown)
			{
				if (vkCode == 'W')
				{
				}
				else if (vkCode == 'S')
				{
				}
				else if (vkCode == 'A')
				{
				}
				else if (vkCode == 'D')
				{
				}
				else if (vkCode == 'Q')
				{
				}
				else if (vkCode == 'E')
				{
				}
				else if (vkCode == 'Z')
				{
				}
				else if (vkCode == 'X')
				{
				}
				else if (vkCode == VK_ESCAPE)
				{
				}
				else if (vkCode == VK_SPACE)
				{
				}
				else if (vkCode == VK_UP)
				{
				}
				else if (vkCode == VK_DOWN)
				{
				}
				else if (vkCode == VK_LEFT)
				{
				}
				else if (vkCode == VK_RIGHT)
				{
				}
			}
		}
		break;

		// Used to repaint area and update it when windows send this message
		case WM_PAINT:
		{
			PAINTSTRUCT Paint = {};
			HDC deviceCtx = BeginPaint(window, &Paint);
			Win32::UpdateWindow(deviceCtx, window, &internalBuffer);
			EndPaint(window, &Paint);
		}
		break;

		case WM_MENUCHAR:
		{
			result = MAKELRESULT(0, MNC_CLOSE);
		}
		break;

		case WM_DESTROY:
		{
			OutputDebugStringA("Window Destroyed\n");
			PostQuitMessage(0);
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
	internal HWND CreateMainWindow(const s32 w, const s32 h, const char *name)
	{
		HINSTANCE instance = nullptr;
		HWND mainWindow = nullptr;

		instance = GetModuleHandleA(nullptr);
		WNDCLASSEXA windowClass = {};

		windowClass.cbSize = sizeof(WNDCLASSEXA);
		windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		windowClass.lpfnWndProc = mainWindowCallback;
		windowClass.hInstance = instance;
		//windowClass.hIcon = LoadIcon(instance, "IDI_WINLOGO");
		windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 6);
		windowClass.lpszClassName = name;
		//windowClass.hIconSm = LoadIcon(windowClass.hInstance, "IDI_ICON");

		const s32 error = RegisterClassExA(&windowClass);
		assert(error != 0 && "Class registration failed");

		RECT rc = {0, 0, static_cast<LONG>(w), static_cast<LONG>(h)};
		AdjustWindowRectEx(&rc, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW);
		const s32 winStyle = WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_OVERLAPPED | WS_SYSMENU;

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

//===============================================XINPUT IMPLEMENTATIONS========================================================

// Defines for interfaces (function pointers) that handles XINPUT, if loading of dll fails then user won't hard crash

// XInputGetState defines -- "define/typedef trick" from Casey Muratori from "handmade hero" :)
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
	typedef X_INPUT_GET_STATE(X_Input_Get_State);
	X_INPUT_GET_STATE(XInputGetStateNotFound)
	{
		return ERROR_DEVICE_NOT_CONNECTED;
	}
	global_variable X_Input_Get_State *xInputGetStatePtr = XInputGetStateNotFound;
#define XInputGetState xInputGetStatePtr

// XInputSetState defines
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
	typedef X_INPUT_SET_STATE(X_Input_Set_State);
	X_INPUT_SET_STATE(XInputSetStateNotFound)
	{
		return ERROR_DEVICE_NOT_CONNECTED;
	}
	global_variable X_Input_Set_State *xInputSetStatePtr = XInputSetStateNotFound;
#define XInputSetState xInputSetStatePtr

	// Used to load xinput dll, if not found then above function pointers will point to "...NotFound" implementation ( return 0 )
	internal void LoadXInputLibrary()
	{
		HMODULE xInputLib = LoadLibraryA("xinput1_4.dll");
		if (xInputLib)
		{
			XInputGetState = (X_Input_Get_State *)GetProcAddress(xInputLib, "XInputGetState");
			if (!XInputGetState)
			{
				XInputGetState = XInputGetStateNotFound;
			}

			XInputSetState = (X_Input_Set_State *)GetProcAddress(xInputLib, "XInputSetState");
			if (!XInputSetState)
			{
				XInputSetState = XInputSetStateNotFound;
			}
		}
	}

	//=============================================MOUSE RAW INPUT===========================================================

	internal void RegisterMouseForRawInput(HWND window = nullptr)
	{
		RAWINPUTDEVICE rawDevices[1];
		// Mouse registering info
		rawDevices[0].usUsagePage = 0x01;
		rawDevices[0].usUsage = 0x02;
		rawDevices[0].dwFlags = 0;
		rawDevices[0].hwndTarget = window;

		if (RegisterRawInputDevices(rawDevices, 1, sizeof(rawDevices[0])) == FALSE)
		{
			//TODO: Proper handling in case of failure to register mouse and/or keyboard
			MessageBoxA(NULL, "Could not register mouse and/or keyboard for raw input", "error", 0);
			assert(0);
		}
	}

	//=============================================XAUDIO 2===========================================================.
	
	#ifndef _XBOX //Little-Endian
	#define fourccRIFF 'FFIR'
	#define fourccDATA 'atad'
	#define fourccFMT ' tmf'
	#define fourccWAVE 'EVAW'
	#define fourccXWMA 'AMWX'
	#define fourccDPDS 'sdpd'
	#endif

	HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD & dwChunkSize, DWORD & dwChunkDataPosition)
	{
		HRESULT hr = S_OK;
		if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, 0, NULL, FILE_BEGIN ) )
			return HRESULT_FROM_WIN32( GetLastError() );

		DWORD dwChunkType;
		DWORD dwChunkDataSize;
		DWORD dwRIFFDataSize = 0;
		DWORD dwFileType;
		DWORD bytesRead = 0;
		DWORD dwOffset = 0;

		while (hr == S_OK)
		{
			DWORD dwRead;
			if( 0 == ReadFile( hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL ) )
				hr = HRESULT_FROM_WIN32( GetLastError() );

			if( 0 == ReadFile( hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL ) )
				hr = HRESULT_FROM_WIN32( GetLastError() );

			switch (dwChunkType)
			{
			case fourccRIFF:
				dwRIFFDataSize = dwChunkDataSize;
				dwChunkDataSize = 4;
				if( 0 == ReadFile( hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL ) )
					hr = HRESULT_FROM_WIN32( GetLastError() );
				break;

			default:
				if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, dwChunkDataSize, NULL, FILE_CURRENT ) )
				return HRESULT_FROM_WIN32( GetLastError() );            
			}

			dwOffset += sizeof(DWORD) * 2;

			if (dwChunkType == fourcc)
			{
				dwChunkSize = dwChunkDataSize;
				dwChunkDataPosition = dwOffset;
				return S_OK;
			}

			dwOffset += dwChunkDataSize;

			if (bytesRead >= dwRIFFDataSize) return S_FALSE;

		}

		return S_OK;

	}

	HRESULT ReadChunkData(HANDLE hFile, void * buffer, DWORD buffersize, DWORD bufferoffset)
	{
		HRESULT hr = S_OK;
		if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, bufferoffset, NULL, FILE_BEGIN ) )
			return HRESULT_FROM_WIN32( GetLastError() );
		DWORD dwRead;
		if( 0 == ReadFile( hFile, buffer, buffersize, &dwRead, NULL ) )
			hr = HRESULT_FROM_WIN32( GetLastError() );
		return hr;
	}
}