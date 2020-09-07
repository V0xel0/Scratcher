#include "types.h"
#include "Win32Platform.h"
#include <omp.h>
#include <xinput.h>

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

	//? For multithreading with omp
	// #pragma omp parallel for
	// for (int i = 0; i < 0; i++) // a dummy loop to start threadpool
	// {
	// }

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
//=============================================INPUT==============================================================================================================


//============================================RENDERING==============================================================================================================
		testRender(&Win32::internalBuffer, XOffset, YOffset);
		Win32::UpdateWindow(deviceContext, window, &Win32::internalBuffer);
		
		++XOffset;
		YOffset += 2;
	}
	UnregisterClassA("Scratcher", GetModuleHandleA(nullptr)); // ? Do we need that?
}