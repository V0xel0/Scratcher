#include "types.h"
#include "window.h"

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	CreateMainWindow(1920, 1080, "Scratcher");
	//Main Program Loop
	bool isRunning = true;
	while(isRunning)
	{
		MSG msg = {};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				isRunning = false;
				break;
			}
		}
	}
	UnregisterClassA("Scratcher", GetModuleHandle(nullptr)); // ? Do we need that?
}