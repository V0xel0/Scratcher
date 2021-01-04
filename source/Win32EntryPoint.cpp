#define _CRT_SECURE_NO_WARNINGS

#include "Utils.hpp"
#include "GameAssertions.hpp"
#include "GameBuildsInterfaces.hpp"
#include "GameService.hpp"

#if UNITY_BUILD
#include "GameService.cpp"
#endif

#include "Win32Platform.hpp"
#include <omp.h>
#include <cstdio>
#include <cassert>

//TODO: Later consider grouping all timestamps and process them in aggregate
inline internal s64 ElapsedMsHere(const s64 startPoint)
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
	Win32::ResizeInternalBuffer(&Win32::internalBuffer, 1280, 720);
	Win32::LoadXInputLibrary();
	Win32::RegisterMouseForRawInput();
	QueryPerformanceFrequency(&Win32::clockFrequency);

	b32 isRunning = true;

	// Timer variables
	LARGE_INTEGER startTime{};
	u64 cycleStart = 0, cycleEnd = 0;

	// Audio stuff

	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	ATL::CComPtr<IXAudio2> XAudio2;
	HRESULT hr;
	hr = XAudio2Create(&XAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	GameAssert((HRESULT)hr >= 0);
	IXAudio2MasteringVoice *pMasterVoice = nullptr;
	hr = XAudio2->CreateMasteringVoice(&pMasterVoice);
	GameAssert((HRESULT)hr >= 0);

	XAUDIO2_BUFFER xaudioBuffer = {};

	const char *strFileName = ("D:\\menu_1.wav");

	HANDLE hFile = CreateFileA(
		strFileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		0);

	LARGE_INTEGER fileSize64;
	GetFileSizeEx(hFile, &fileSize64);

	u32 fileSize32 = [](u64 val) {
		GameAssert(val <= UINT_MAX);
		u32 Result = (u32)val;
		return (Result);
	}(fileSize64.QuadPart);

	BYTE *fileBuffer = new BYTE[fileSize32];

	[](HANDLE hFile, void *fileBuffer, u32 fileSize32) {
		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD bytesRead;
			if (ReadFile(hFile, fileBuffer, fileSize32, &bytesRead, 0) && bytesRead == fileSize32)
			{
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
	}(hFile, fileBuffer, fileSize32);

	auto &&[wfx, wavData, wavDataSize] = [](void *wavMemory) {
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
			//TODO: Loops are not safe idea but .wav can have anything between end of fmt and start of data
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
	}(fileBuffer);

	xaudioBuffer.AudioBytes = wavDataSize;		//size of the audio buffer in bytes
	xaudioBuffer.pAudioData = wavData;			//buffer containing audio data
	xaudioBuffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer
	xaudioBuffer.LoopCount = XAUDIO2_LOOP_INFINITE;

	// Create Source Voice and submit buffer to it
	IXAudio2SourceVoice *pSourceVoice;
	hr = XAudio2->CreateSourceVoice(&pSourceVoice, (WAVEFORMATEX *)wfx);
	GameAssert((HRESULT)hr >= 0);
	hr = pSourceVoice->SubmitSourceBuffer(&xaudioBuffer);
	GameAssert((HRESULT)hr >= 0);

	// Activate Source voice
	hr = pSourceVoice->Start();
	GameAssert((HRESULT)hr >= 0);

	// Providing memory for game
	//TODO: Consider big reserve and then commit as grow/needed for eventual editor
	GameMemory gameMemory = {};
	gameMemory.PermanentStorageSize = MiB(64);
	gameMemory.TransientStorageSize = GiB(4);

	// Check available memory
	MEMORYSTATUSEX memStatus = {};
	memStatus.dwLength = sizeof(memStatus);
	GlobalMemoryStatusEx(&memStatus);
	u64 availablePhysicalMemory = memStatus.ullAvailPhys;
	u64 maxMemorySize = gameMemory.PermanentStorageSize + gameMemory.TransientStorageSize;
	AlwaysAssert(maxMemorySize < availablePhysicalMemory && "Download more RAM");

	gameMemory.PermanentStorage = VirtualAlloc(nullptr, maxMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	AlwaysAssert(gameMemory.PermanentStorage && "Failed to allocate memory from Windows");
	gameMemory.TransientStorage = (byte *)gameMemory.PermanentStorage + gameMemory.PermanentStorageSize;

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
		// Fill game services
		GameScreenBuffer gameBuffer = {};
		gameBuffer.height = Win32::internalBuffer.height;
		gameBuffer.width = Win32::internalBuffer.width;
		gameBuffer.pitch = Win32::internalBuffer.pitch;
		gameBuffer.memory = Win32::internalBuffer.memory;

		// Update
		GameFullUpdate(&gameMemory, &gameBuffer);
		Win32::UpdateWindow(deviceContext, window, &Win32::internalBuffer);

		// Timers
		s64 frameTimeMs = ElapsedMsHere(startTime.QuadPart);
		cycleEnd = __rdtsc();
		u64 mcpf = (cycleStart - cycleEnd) / (1'000'000);

		// Temporary for debug
		char tbuffer[32];
		sprintf(tbuffer, "Ms: %lld\n", frameTimeMs);
		OutputDebugStringA(tbuffer);
	}
	UnregisterClassA("Scratcher", GetModuleHandleA(nullptr)); // ? Do we need that?
	CoUninitialize();
}