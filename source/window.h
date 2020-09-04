#pragma once
#include "platform.h"
#include "types.h"

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

	HWND CreateMainWindow(const int w, const int h, const char* name = "Default");
	void CreateScreenBuffer(ScreenBuffer *w32Buffer, const s32 w, const s32 h);
	void UpdateWindow(HDC deviceCtx, s32 width, s32 height, ScreenBuffer w32Buffer);
	WindowDimensions GetWindowClientDimensions(HWND window);
}