#include "types.h"
#include "window.h"

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND window = CreateMainWindow(1920, 1080, "Scratcher");
	HDC DeviceContext = GetDC(window);

	//Main Program Loop
	bool isRunning = true;

	s32 XOffset = 0;
	s32 YOffset = 0;

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

		renderSomeGradient(XOffset, YOffset);

		RECT ClientRect;
		GetClientRect(window, &ClientRect);
		Win32::UpdateWindow(DeviceContext, &ClientRect);
		
		++XOffset;
		YOffset += 2;
	}
	ReleaseDC(window, DeviceContext);
	UnregisterClassA("Scratcher", GetModuleHandleA(nullptr)); // ? Do we need that?
}