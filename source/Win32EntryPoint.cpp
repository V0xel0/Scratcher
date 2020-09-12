#define _CRT_SECURE_NO_WARNINGS
#include "types.h"
#include "Win32Platform.h"
#include <omp.h>
#include <cstdio>

// TEST-ONLY-FUNCTION for checking basic pixel drawing & looping
void testRender(Win32::ScreenBuffer *w32Buffer, const s32 offsetX, const s32 offsetY)
{
	s32 width = w32Buffer->width;
	s32 height = w32Buffer->height;

	u8 *row = (u8 *)w32Buffer->memory;
#if 1
	for (s32 y = 0; y < height; y++)
	{
		u32 *pixel = (u32 *)row;
		for (s32 x = 0; x < width; x++)
		{
			u8 b = (u8)(x + offsetX);
			u8 g = (u8)(y + offsetY);
			u8 r = (u8)Win32::mouseData.x;

			*pixel++ = ((r << 16) | (g << 8) | b);
		}
		row += w32Buffer->pitch;
	}
#endif
	//? Multithreaded, but single core is slower cause of division and modulo
#if 0
	omp_set_num_threads(12);
#pragma omp parallel for
	for (int xy = 0; xy < width*height; ++xy) 
	{
		s32 x = xy % width;
		s32 y = xy / width;
		
		u32 *pixel = ( (u32*)row ) + xy;
		u8 b = (u8)(x + offsetX);
		u8 g = (u8)(y + offsetY);
		u8 r = (u8)Win32::mouseData.x;
		*pixel = ((r << 16) |(g << 8) | b);
	}
#endif
}

inline internal s64 ElapsedMsHere(s64 startPoint)
{
	LARGE_INTEGER hereEnd = {};
	s64 elapsedMs = 0;
	QueryPerformanceCounter(&hereEnd);
	elapsedMs = startPoint - hereEnd.QuadPart;
	elapsedMs *= 1000;
	elapsedMs /= Win32::clockFrequency.QuadPart;
	return elapsedMs;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	//?	Consider "inline style" (would be great to have named, capture scopes) as discussed by John Carmack here:
	//?	http://number-none.com/blow/blog/programming/2014/09/26/carmack-on-inlined-code.html

	// Creation and initalization of platform data and interfaces
	HWND window = Win32::CreateMainWindow(1920, 1080, "Scratcher");
	HDC deviceContext = GetDC(window);
	Win32::ResizeInternalBuffer(&Win32::internalBuffer, 1920, 1080);
	Win32::LoadXInputLibrary();
	Win32::RegisterMouseForRawInput();
	QueryPerformanceFrequency(&Win32::clockFrequency);

	b32 isRunning = true;
	s32 XOffset = 0;
	s32 YOffset = 0;

	// Timer variables
	LARGE_INTEGER startTime{};
	u64 cycleStart = 0, cycleEnd = 0;
	
	// Main Win32 platform loop
	while (isRunning)
	{
		QueryPerformanceCounter(&startTime);
		cycleStart = __rdtsc();
		
		MSG msg = {};
		while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
			if (msg.message == WM_QUIT)
			{
				isRunning = false;
				break;
			}
		}
#if 0
		//=============================================XINPUT============================================================
		//TODO: Handle deadzone
		for (DWORD gamePadID = 0; gamePadID < XUSER_MAX_COUNT; gamePadID++)
		{
			XINPUT_STATE gamePadState = {};
			
			if( Win32::XInputGetState(gamePadID, &gamePadState) == ERROR_SUCCESS )
			{
				XINPUT_GAMEPAD *gamePad = &gamePadState.Gamepad;

				b32 up = (gamePad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
				b32 down = (gamePad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
				b32 left = (gamePad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
				b32 right = (gamePad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
				b32 start = (gamePad->wButtons & XINPUT_GAMEPAD_START);
				b32 back = (gamePad->wButtons & XINPUT_GAMEPAD_BACK);
				b32 leftShoulder = (gamePad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
				b32 rightShoulder = (gamePad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
				b32 aButton = (gamePad->wButtons & XINPUT_GAMEPAD_A);
				b32 bButton = (gamePad->wButtons & XINPUT_GAMEPAD_B);
				b32 xButton = (gamePad->wButtons & XINPUT_GAMEPAD_X);
				b32 yButton = (gamePad->wButtons & XINPUT_GAMEPAD_Y);
								
				s16 triggerL = gamePad->sThumbLX;
				s16 triggerR = gamePad->sThumbLY;

				XOffset += triggerL >> 12;
				YOffset -= triggerR >> 12;
			}
			else
			{
				// Controller is not connected
			}
		}
#endif
		XOffset += Win32::mouseData.lastDx;
		YOffset += Win32::mouseData.lastDy;
		Win32::mouseData.lastDx = 0;
		Win32::mouseData.lastDy = 0;

		//============================================RENDERING=============================
		testRender(&Win32::internalBuffer, XOffset, YOffset);
		Win32::UpdateWindow(deviceContext, window, &Win32::internalBuffer);

		++XOffset;
		YOffset += 2;

		s64 frameTimeMs = ElapsedMsHere(startTime.QuadPart);
		cycleEnd = __rdtsc();
		u64 mcpf = (cycleStart-cycleEnd) / (1'000'000);

		char buffer[32];
		sprintf(buffer, "Ms: %lld\n", frameTimeMs);
		OutputDebugStringA(buffer);
	}
	UnregisterClassA("Scratcher", GetModuleHandleA(nullptr)); // ? Do we need that?
}