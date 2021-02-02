#pragma once
// Windows 10
#define _WIN32_WINNT 0x0A00
#define NOMINMAX
// Include <mcx.h> if needed
#define NOMCX
// Include <winsvc.h> if needed
#define NOSERVICE
// WinHelp is deprecated
#define NOHELP
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <Xinput.h>
#include <xaudio2.h>

//? Win32 Platform layer implementations, intended to be used with "WINAPI WinMain" only!
//? In order to provide distinction from Microsoft's WinApi functions "Win32" namespace is
//? provided for all custom platform layer functions and structures
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

		f32 widthDelta;
		f32 heightDelta;
	};

	struct XAudioCustomBuffer
	{
		XAUDIO2_BUFFER buffer;
		WAVEFORMATEXTENSIBLE *wfx;
	};

	struct DynamicGameCode
	{
		HMODULE gameCodeDLL;
		FILETIME lastChangeTime;
		gameFullUpdatePtr *update;
		b32 isValid;
	};

	struct PlatformState
	{
		u64 totalSize;
		void *gameMemory;

		HANDLE recordingHandle;
		s32 recordID;

		HANDLE playbackHandle;
		s32 playbackID;

		const char *recordInputFileName;
	};

	// ========================================== INTERNAL GLOBALS =================================================================

	//? Internal globals are never exposed to application layer directly, they are mostly data that
	//? needs to be shared with window CALLBACK function in Windows

	global_variable b32 isMainRunning = true;
	global_variable LARGE_INTEGER clockFrequency;

	//? One 4 bytes per pixel buffer with one not shared device context is assumed
	constexpr global_variable s32 bytesPerPixel = 4;
	global_variable ScreenBuffer internalBuffer = {};

	// ======================================== WINDOW AND GRAPHICS BUFFER =========================================================

	// Used for getting dimensions of client area for passed window
	internal auto GetWindowClientDimensions(HWND window)
	{
		struct Output { s32 w; s32 h; } out;
		RECT rect;
		GetClientRect(window, &rect);
		out.w = rect.right - rect.left;
		out.h = rect.bottom - rect.top;
		return out;
	}

	// Used for updating window data when WM_PAINT msg from windows appears or when platform layer wants to update
	// It copies the color data from buffer rectangle to client area of a window, it also stretches the buffer to fit the client area
	internal void UpdateWindow(HDC deviceCtx, HWND window, ScreenBuffer *buffer)
	{
		GameAssert(buffer != nullptr);
		auto&&[clientAreaWidth, clientAreaHeight] = Win32::GetWindowClientDimensions(window);

		//TODO: BitBlt might be faster
		StretchDIBits(
			deviceCtx,
			0, 0, clientAreaWidth, clientAreaHeight,
			0, 0, buffer->width, buffer->height,
			buffer->memory, &buffer->info,
			DIB_RGB_COLORS, SRCCOPY);
	}

	// Windows callback functions for window messages processing. It is being called by "DispatchMessage" or directly by Windows
	LRESULT CALLBACK mainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		LRESULT output = 0;

		switch (message)
		{
		// Used to repaint area and update it when windows send this message
		case WM_PAINT:
		{
			PAINTSTRUCT Paint = {};
			HDC deviceCtx = BeginPaint(window, &Paint);
			Win32::UpdateWindow(deviceCtx, window, &internalBuffer);
			EndPaint(window, &Paint);
		}
		break;

		case WM_SIZE:
		{
			auto&&[clientAreaWidth, clientAreaHeight] = Win32::GetWindowClientDimensions(window);
			internalBuffer.widthDelta = (f32)clientAreaWidth/internalBuffer.width;
			internalBuffer.heightDelta = (f32)clientAreaHeight/internalBuffer.height;
		}
		break;

		case WM_MENUCHAR:
		{
			output = MAKELRESULT(0, MNC_CLOSE);
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
			output = DefWindowProc(window, message, wParam, lParam);
		}
		break;
		}
		return (output);
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
		windowClass.lpfnWndProc = Win32::mainWindowCallback;
		windowClass.hInstance = instance;
		//windowClass.hIcon = LoadIcon(instance, "IDI_WINLOGO");
		windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 6);
		windowClass.lpszClassName = name;
		//windowClass.hIconSm = LoadIcon(windowClass.hInstance, "IDI_ICON");

		const s32 error = RegisterClassExA(&windowClass);
		GameAssert(error != 0 && "Class registration failed");

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

		GameAssert(mainWindow != nullptr && "Window creation failed");

		ShowWindow(mainWindow, SW_SHOW);
		SetForegroundWindow(mainWindow);
		SetFocus(mainWindow);

		return mainWindow;
	}

	//TODO: Change allocation model to not allocate and just get memory from outside or if not then consider wrapping to RAII
	// Used to create a new Win32 Screen Buffer with 4 bytes pixels with BGRA memory order
	internal void ResizeInternalBuffer(ScreenBuffer *buffer, const s32 w, const s32 h)
	{
		GameAssert(buffer != nullptr);
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
	// ========================================= INPUT & GAME STATE RECORDING ============================================================

	//TODO: Use additional memory and write lazily to disk instead?
	internal void beginInputRecording(PlatformState *state, const s32 inputRecordID)
	{
		state->recordID = inputRecordID;
		state->recordingHandle = CreateFileA(state->recordInputFileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

		//TODO: Save state only instead of whole memory?
		DWORD bytesToWrite = (DWORD)state->totalSize;
		AlwaysAssert(state->totalSize == bytesToWrite);
		DWORD bytesWritten;
		WriteFile(state->recordingHandle, state->gameMemory, bytesToWrite, &bytesWritten, 0);
	}

	internal void endInputRecording(PlatformState *state)
	{
		CloseHandle(state->recordingHandle);
		state->recordID = 0;
	}

	internal void beginInputPlayback(PlatformState *state, s32 inputPlaybackID)
	{
		state->playbackID = inputPlaybackID;
		state->playbackHandle = CreateFileA(state->recordInputFileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

		DWORD bytesToRead = (DWORD)state->totalSize;
		AlwaysAssert(state->totalSize == bytesToRead);
		DWORD bytesRead;
		ReadFile(state->playbackHandle, state->gameMemory, bytesToRead, &bytesRead, 0);
	}

	internal void endInputPlayback(PlatformState *state)
	{
		CloseHandle(state->playbackHandle);
		state->playbackID = 0;
	}

	internal void recordInput(PlatformState *state, GameInput *newInput)
	{
		DWORD bytesWritten = 0;
		WriteFile(state->recordingHandle, newInput, sizeof(*newInput), &bytesWritten, 0);
	}

	internal void playBackInput(PlatformState *state, GameInput *newInput)
	{
		DWORD bytesRead = 0;
		if(ReadFile(state->playbackHandle, newInput, sizeof(*newInput), &bytesRead, 0))
		{
			// After reaching end, start from beginning
			if(bytesRead == 0)
			{
				s32 playID = state->playbackID;
				Win32::endInputPlayback(state);
				Win32::beginInputPlayback(state, playID);
				// Read input again after reading game state
				ReadFile(state->playbackHandle, newInput, sizeof(*newInput), &bytesRead, 0);
			}
		}
	}
	// =========================================== MOUSE & KEYBOARD EVENTS HANDLING ==================================================

	internal void processKeyboardMouseEvent(GameKeyState *newState, const b32 isDown)
	{
		if(newState->wasDown != isDown)
		{
			newState->wasDown = isDown;
			++newState->halfTransCount;
		}
	}

	internal void processKeyboardMouseMsgs(PlatformState *state, MSG *msg, GameController *keyboardMouse)
	{
		LPARAM lParam = msg->lParam;
		WPARAM wParam = msg->wParam;

		switch (msg->message)
		{
			// Used for obtaining relative mouse movement and wheel without acceleration
			case WM_INPUT:
			{
				u32 size = sizeof(RAWINPUT);
				RAWINPUT raw[sizeof(RAWINPUT)];
				u32 copied = GetRawInputData( (HRAWINPUT)lParam, RID_INPUT, raw, &size, sizeof(RAWINPUTHEADER) );
				
				if (copied != size)
				{
					//TODO: Fail handling from GetRawInputData()
					MessageBoxA(NULL, "Incorrect raw input data size!", "error", 0);
					break;
				}
				if (raw->header.dwType == RIM_TYPEMOUSE )
				{
					keyboardMouse->mouse.deltaX = raw->data.mouse.lLastX;
					keyboardMouse->mouse.deltaY = raw->data.mouse.lLastY;
					if (raw->data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
					{
						keyboardMouse->mouse.deltaWheel = (*(s16 *)&raw->data.mouse.usButtonData);
					}
				}
			}
			break;

			case WM_MOUSEMOVE:
			{
				keyboardMouse->mouse.x = (s32)(GET_X_LPARAM(lParam)/Win32::internalBuffer.widthDelta);
				keyboardMouse->mouse.y = (s32)(GET_Y_LPARAM(lParam)/Win32::internalBuffer.heightDelta);
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
				u32 vkCode = (u32)wParam;
				b32 wasDown = TestBit(lParam, 30) != 0;
				b32 isDown = TestBit(lParam, 31) == 0;
				//TODO: Consider binding from file?
				if (wasDown != isDown)
				{
					if (vkCode == 'W')
					{
						Win32::processKeyboardMouseEvent(&keyboardMouse->moveUp, isDown);
					}
					else if (vkCode == 'S')
					{
						Win32::processKeyboardMouseEvent(&keyboardMouse->moveDown, isDown);
					}
					else if (vkCode == 'A')
					{
						Win32::processKeyboardMouseEvent(&keyboardMouse->moveLeft, isDown);
					}
					else if (vkCode == 'D')
					{
						Win32::processKeyboardMouseEvent(&keyboardMouse->moveRight, isDown);
					}
					else if (vkCode == 'Q')
					{
						Win32::processKeyboardMouseEvent(&keyboardMouse->action1, isDown);
					}
					else if (vkCode == 'E')
					{
						Win32::processKeyboardMouseEvent(&keyboardMouse->action2, isDown);
					}
					else if (vkCode == 'Z')
					{
						Win32::processKeyboardMouseEvent(&keyboardMouse->action3, isDown);
					}
					else if (vkCode == 'X')
					{
						Win32::processKeyboardMouseEvent(&keyboardMouse->action4, isDown);
					}
					else if (vkCode == VK_ESCAPE)
					{
						Win32::processKeyboardMouseEvent(&keyboardMouse->back, isDown);
					}
					else if (vkCode == VK_RETURN)
					{
						Win32::processKeyboardMouseEvent(&keyboardMouse->start, isDown);
					}
					else if (vkCode == VK_SPACE)
					{
						Win32::processKeyboardMouseEvent(&keyboardMouse->actionFire, isDown);
					}
					else if (vkCode == VK_UP)
					{
						Win32::processKeyboardMouseEvent(&keyboardMouse->moveUp, isDown);
					}
					else if (vkCode == VK_DOWN)
					{
						Win32::processKeyboardMouseEvent(&keyboardMouse->moveDown, isDown);
					}
					else if (vkCode == VK_LEFT)
					{
						Win32::processKeyboardMouseEvent(&keyboardMouse->moveLeft, isDown);
					}
					else if (vkCode == VK_RIGHT)
					{
						Win32::processKeyboardMouseEvent(&keyboardMouse->moveRight, isDown);
					}
					#if GAME_INTERNAL
					else if (vkCode == 'L')
					{
						if(isDown)
						{
							if(state->playbackID == 0)
							{
								if(state->recordID == 0)
								{
									Win32::beginInputRecording(state, 1);
								}
								else
								{
									Win32::endInputRecording(state);
									Win32::beginInputPlayback(state, 1);
								}
							}
							else
							{
								Win32::endInputPlayback(state);
							}
						}
					}	
					#endif
				}
				b32 altWasDown = TestBit(lParam, 29);
				if ((vkCode == VK_F4) && altWasDown)
				{
					Win32::isMainRunning = false;
				}
				break;
			}
			default:
			{
			}
			break;
		}
	}

	// =============================================== XINPUT ========================================================

	internal void processXInputDigitalEvent(DWORD xInputButtonState, GameKeyState *oldState, DWORD buttonBit,
											GameKeyState *newState)
	{
		newState->wasDown = ((xInputButtonState & buttonBit) == buttonBit);
		newState->halfTransCount = (oldState->wasDown != newState->wasDown) ? 1 : 0;
	}

	internal f32 processXInputAnalogEvent(SHORT value, SHORT deadZoneThreshold)
	{
		f32 output = 0;
		// Normalization of analog data and deadzone handling
		if (value < -deadZoneThreshold)
		{
			output = (f32)((value + deadZoneThreshold) / (32768.0f - deadZoneThreshold));
		}
		else if (value > deadZoneThreshold)
		{
			output = (f32)((value - deadZoneThreshold) / (32767.0f - deadZoneThreshold));
		}
		return output;
	}

	//? Defines for interfaces (function pointers) that handles XINPUT, if loading of dll fails then user won't hard crash
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
	// =============================================== MOUSE RAW INPUT ===========================================================

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
			GameAssert(0);
		}
	}
	// =============================================== UTILITIES ==============================================================

	internal auto parseWaveData(void *wavMemory)
	{
		if (wavMemory == nullptr)
			//TODO: Log/Error
			GameAssert(0);

		byte *seek = (byte *)wavMemory;
		u32 riffString = *(u32 *)seek;
		u32 waveString = *(u32 *)(seek + 8);

		struct Output
		{
			WAVEFORMATEXTENSIBLE *wfx;
			byte *data;
			u32 dataSize;
		} out;

		if (riffString == 'FFIR' && waveString == 'EVAW')
		{
			//TODO: Bulletproof loops in case of wrong/malicious wave data
			u32 smallOffset = sizeof(u16);
			u32 bigOffset = smallOffset * 4;

			while (*(u32 *)seek != ' tmf')
				seek += smallOffset;

			out.wfx = (WAVEFORMATEXTENSIBLE *)(seek + bigOffset);
			u32 fmtSize = *((u32 *)seek + 1);
			seek += fmtSize;

			while (*(u32 *)seek != 'atad')
				seek += smallOffset;

			out.data = (seek + bigOffset);
			out.dataSize = *((u32 *)seek + 1);
		}
		else
		{
			//TODO: Log/Error
			GameAssert(0 && "Not a .wav file!");
		}
		return out;
	}

	internal s32 getMonitorFrequency()
	{
		DEVMODEA devInfo = {};
		devInfo.dmSize = sizeof(DEVMODE);
		EnumDisplaySettingsA(0 , ENUM_CURRENT_SETTINGS, &devInfo);
		return devInfo.dmDisplayFrequency;
	}
	// ================================================== GAME HOT-RELOADING =========================================================

	internal FILETIME getFileWriteTime(const char *name)
	{
		FILETIME time = {};
		WIN32_FILE_ATTRIBUTE_DATA data;
		if (GetFileAttributesExA(name, GetFileExInfoStandard, &data ))
		{
			time = data.ftLastWriteTime;
		}
		return time;
	}
	
	internal DynamicGameCode loadGameCode(const char *dllName, const char *tempDllName, const char *lockFilePath)
	{
		DynamicGameCode output = {};
		WIN32_FILE_ATTRIBUTE_DATA ignored;
		if (!GetFileAttributesExA(lockFilePath, GetFileExInfoStandard, &ignored))
		{
			output.lastChangeTime = Win32::getFileWriteTime(dllName);
			CopyFileA(dllName, tempDllName, FALSE);
			output.gameCodeDLL = LoadLibraryA(tempDllName);

			if(output.gameCodeDLL)
			{
				output.update = (gameFullUpdatePtr *)GetProcAddress(output.gameCodeDLL, "gameFullUpdate");
				output.isValid = (output.update != nullptr);
			}
		}
		if(!output.isValid)
		{
			output.update = gameFullUpdateNotLoaded;
		}

		return output;
	}

	internal void unloadGameCode(DynamicGameCode *gameCode)
	{
		GameAssert(gameCode != 0 && "Trying to free invalid game code!");
		if(gameCode->gameCodeDLL)
		{
			FreeLibrary(gameCode->gameCodeDLL);
			gameCode->gameCodeDLL = 0;
		}
		gameCode->isValid = false;
		gameCode->update = gameFullUpdateNotLoaded;
	}
// ================================================= DEBUG INTERNAL FUNCTIONS ==========================================================

#if GAME_INTERNAL
	DebugFileOutput debugReadFile(char *fileName)
	{
		HANDLE fileHandle = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
		auto d = deferFunction([&] { CloseHandle(fileHandle); });
		DebugFileOutput out = {};

		if (fileHandle != INVALID_HANDLE_VALUE)
		{
			LARGE_INTEGER fileSize;

			if (GetFileSizeEx(fileHandle, &fileSize))
			{
				u32 fileSize32 = truncU64toU32(fileSize.QuadPart);
				out.data = VirtualAlloc(0, fileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

				if (out.data)
				{
					DWORD bytesRead;
					if (ReadFile(fileHandle, out.data, fileSize32, &bytesRead, 0) && (fileSize32 == bytesRead))
					{
						out.dataSize = fileSize32;
					}
					else
					{
						VirtualFree(out.data, 0, MEM_RELEASE);
						out.data = nullptr;
						//TODO: Log
					}
				}
			}
		}
		else
		{
			//TODO: Log
		}

		return (out);
	}

	b32 debugWriteFile(char *fileName, void *memory, u32 memSize)
	{
		b32 isSuccessful = false;
		HANDLE fileHandle = CreateFileA(fileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
		auto d = deferFunction([&] { CloseHandle(fileHandle); });

		if (fileHandle != INVALID_HANDLE_VALUE)
		{
			DWORD bytesWritten;
			if (WriteFile(fileHandle, memory, memSize, &bytesWritten, 0))
			{
				isSuccessful = (bytesWritten == memSize);
			}
			else
			{
				//TODO: Log
			}
		}
		else
		{
			//TODO: Log
		}

		return isSuccessful;
	}

	void debugFreeFileMemory(void *memory)
	{
		if (memory != nullptr)
		{
			VirtualFree(memory, 0, MEM_RELEASE);
		}
	}
#endif


} // namespace Win32