#include "types.h"
#include "Win32Platform.h"
#include <omp.h>


// TEST-ONLY-FUNCTION for checking basic pixel drawing & looping
void testRender(Win32::ScreenBuffer *w32Buffer, const s32 offsetX, const s32 offsetY)
{
	s32 width = w32Buffer->width;
	s32 height = w32Buffer->height;

	u8 *row = (u8*)w32Buffer->memory;
#if 1
	for (s32 y = 0; y < height; y++)
	{
		u32 *pixel = (u32*)row;
		for (s32 x = 0; x < width; x++)
		{
			u8 b = (u8)(x + offsetX);
			u8 g = (u8)(y + offsetY);
			u8 r = 255;

			*pixel++ = ((r << 16) | (g << 8) | b);
		}
		row += w32Buffer->pitch;
	}
#endif
	//? Multithreaded, but single core is slower cause of division and modulo
#if 0
	omp_set_num_threads(12);
	#pragma omp parallel for
	for (int xy = 0; xy < bitmapWidth*bitmapHeight; ++xy) 
	{
		s32 x = xy % bitmapWidth;
		s32 y = xy / bitmapWidth;
		
		u32 *pixel = ( (u32*)row ) + xy;
		u8 b = (u8)(x + offsetX);
		u8 g = (u8)(y + offsetY);
		u8 r = 255;
		*pixel = ((r << 16) |(g << 8) | b);
	}
#endif
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND window = Win32::CreateMainWindow(1920, 1080, "Scratcher");
	HDC deviceContext = GetDC(window);
	Win32::ResizeInternalBuffer(&Win32::internalBuffer, 1920, 1080);
	Win32::LoadXInputLibrary();
	Win32::RegisterMouseForRawInput();
	
	bool isRunning = true;
	s32 XOffset = 0;
	s32 YOffset = 0;

	// Main Win32 platform loop
	while(isRunning)
	{
		MSG msg = {};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
			if (msg.message == WM_QUIT)
			{
				isRunning = false;
				break;
			}
		}

		//=============================================XINPUT============================================================
		//TODO: Handle deadzone
		for (DWORD gamePadID = 0; gamePadID < XUSER_MAX_COUNT; gamePadID++)
		{
			XINPUT_STATE gamePadState = {};
			
			if( Win32::XInputGetState(gamePadID, &gamePadState) == ERROR_SUCCESS )
			{
				XINPUT_GAMEPAD *gamePad = &gamePadState.Gamepad;

				bool up = (gamePad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
				bool down = (gamePad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
				bool left = (gamePad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
				bool right = (gamePad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
				bool start = (gamePad->wButtons & XINPUT_GAMEPAD_START);
				bool back = (gamePad->wButtons & XINPUT_GAMEPAD_BACK);
				bool leftShoulder = (gamePad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
				bool rightShoulder = (gamePad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
				bool aButton = (gamePad->wButtons & XINPUT_GAMEPAD_A);
				bool bButton = (gamePad->wButtons & XINPUT_GAMEPAD_B);
				bool xButton = (gamePad->wButtons & XINPUT_GAMEPAD_X);
				bool yButton = (gamePad->wButtons & XINPUT_GAMEPAD_Y);
								
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

		//============================================RENDERING===================================================================
		testRender(&Win32::internalBuffer, XOffset, YOffset);
		Win32::UpdateWindow(deviceContext, window, &Win32::internalBuffer);
		
		//++XOffset;
		//YOffset += 2;
	}
	UnregisterClassA("Scratcher", GetModuleHandleA(nullptr)); // ? Do we need that?
}