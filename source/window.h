#pragma once
#include "platform.h"
#include "types.h"
HWND CreateMainWindow(const int w, const int h, const char* name = "Default");

//! TODO: ONLY NOW FOR TESTS, LATER MOVE IT OUT!
void renderSomeGradient(const s32 offsetX, const s32 offsetY);
namespace Win32
{
	void UpdateWindow(HDC deviceCtx, RECT *clientRect);
}