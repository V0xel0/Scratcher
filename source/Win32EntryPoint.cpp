#define _CRT_SECURE_NO_WARNINGS

#include "GameAssertions.hpp"
#include "Utils.hpp"
#include "GameService.hpp"
#include "Win32Platform.hpp"

#if UNITY_BUILD
#include "GameService.cpp"
#endif

#include <omp.h>
#include <cstdio>

//TODO: Later consider grouping all timestamps and process them in aggregate
inline internal f32 ElapsedMsHere(const s64 startPoint)
{
	LARGE_INTEGER hereEnd = {};
	f32 elapsedMs = 0.0;
	QueryPerformanceCounter(&hereEnd);
	elapsedMs =  (f32)(hereEnd.QuadPart - startPoint);
	elapsedMs *= 1000;
	elapsedMs /= Win32::clockFrequency.QuadPart;
	return elapsedMs;
}

s32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, s32 nCmdShow)
{
	//?	Consider "inline style" (would be great to have named, capture scopes) as discussed by John Carmack here:
	//?	http://number-none.com/blow/blog/programming/2014/09/26/carmack-on-inlined-code.html

	// Creation and initalization of platform data and interfaces
	UINT schedulerGranularity = 1;
	b32 schedulerError = (timeBeginPeriod(schedulerGranularity) == TIMERR_NOERROR);
	GameAssert(schedulerError);
	
	HWND window = Win32::CreateMainWindow(1920, 1080, "Scratcher");
	HDC deviceContext = GetDC(window);
	Win32::ResizeInternalBuffer(&Win32::internalBuffer, 1280, 720);
	Win32::LoadXInputLibrary();
	Win32::RegisterMouseForRawInput();
	QueryPerformanceFrequency(&Win32::clockFrequency);
	
	s32 monitorRefresh = Win32::getMonitorFrequency();
	f32 targetFrequencyRate = 33.333333f;

	// Timer variables
	LARGE_INTEGER startFrameCounter = {};
	u64 cycleStart = 0, cycleEnd = 0;

	// Creation of Buffer for previous and current controllers state
	GameInput gameInputBuffer[2] = {};
	GameInput *newInputs = &gameInputBuffer[0];
	GameInput *oldInputs = &gameInputBuffer[1];

	// Audio init
	IXAudio2 *XAudio2;
	HRESULT hr;
	hr = XAudio2Create(&XAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	GameAssert((HRESULT)hr >= 0);
	IXAudio2MasteringVoice *pMasterVoice = nullptr;
	hr = XAudio2->CreateMasteringVoice(&pMasterVoice);
	GameAssert((HRESULT)hr >= 0);
	//TODO: Consider informing game about limits or adjust platform to game
	constexpr s32 maxAudioSources = 64;
	constexpr s32 maxActiveSounds = 64;
	s32 nextFreeVoiceID = 0;
	Win32::XAudioCustomBuffer xaudioCustomBuffers[maxAudioSources] = {};
	IXAudio2SourceVoice *sourceVoices[maxActiveSounds] = {};

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

	QueryPerformanceCounter(&startFrameCounter);

	// Main Win32 platform loop
	while (Win32::isMainRunning)
	{
		cycleStart = __rdtsc();
		MSG msg = {};

		//TODO: More explicitly indicate controllers IDs
		// Handling of Keyboard + mouse controller
		GameController *oldKeyboardMouseController = getGameController(oldInputs, 0);
		GameController *newKeyboardMouseController = getGameController(newInputs, 0);
		*newKeyboardMouseController = {};
		newKeyboardMouseController->isConnected = true;
		for (s32 i = 0; i < ArrayCount32(newKeyboardMouseController->buttons); ++i)
		{
			newKeyboardMouseController->buttons[i].wasDown = oldKeyboardMouseController->buttons[i].wasDown;
		}

		while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			Win32::processKeyboardMouseMsgs(&msg, newKeyboardMouseController);
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
			if (msg.message == WM_QUIT)
			{
				Win32::isMainRunning = false;
				break;
			}
		}

		//=============================================XINPUT============================================================
		s32 maxActiveGamePads = min((s32)XUSER_MAX_COUNT, ArrayCount32(newInputs->controllers) - 1);
		for (s32 gamePadID = 1; gamePadID <= maxActiveGamePads; gamePadID++)
		{
			GameController *oldGamePadController = getGameController(oldInputs, gamePadID);
			GameController *newGamePadController = getGameController(newInputs, gamePadID);
			XINPUT_STATE gamePadState = {};

			if (Win32::XInputGetState(gamePadID - 1, &gamePadState) == ERROR_SUCCESS)
			{
				newGamePadController->isConnected = true;
				newGamePadController->isGamePad = true;
				XINPUT_GAMEPAD *gamePad = &gamePadState.Gamepad;

				newGamePadController->gamePad.StickAverageX = Win32::processXInputAnalogEvent(gamePad->sThumbLX,
																							  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
				newGamePadController->gamePad.StickAverageY = Win32::processXInputAnalogEvent(gamePad->sThumbLY,
																							  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

				Win32::processXInputDigitalEvent(gamePad->wButtons, &oldGamePadController->moveUp,
												 XINPUT_GAMEPAD_DPAD_UP, &newGamePadController->moveUp);
				Win32::processXInputDigitalEvent(gamePad->wButtons, &oldGamePadController->moveDown,
												 XINPUT_GAMEPAD_DPAD_DOWN, &newGamePadController->moveDown);
				Win32::processXInputDigitalEvent(gamePad->wButtons, &oldGamePadController->moveLeft,
												 XINPUT_GAMEPAD_DPAD_LEFT, &newGamePadController->moveLeft);
				Win32::processXInputDigitalEvent(gamePad->wButtons, &oldGamePadController->moveRight,
												 XINPUT_GAMEPAD_DPAD_RIGHT, &newGamePadController->moveRight);
				Win32::processXInputDigitalEvent(gamePad->wButtons, &oldGamePadController->actionFire,
												 XINPUT_GAMEPAD_B, &newGamePadController->actionFire);

				// b32 start = (gamePad->wButtons & XINPUT_GAMEPAD_START);
				// b32 back = (gamePad->wButtons & XINPUT_GAMEPAD_BACK);
				// b32 leftShoulder = (gamePad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
				// b32 rightShoulder = (gamePad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
				// b32 aButton = (gamePad->wButtons & XINPUT_GAMEPAD_A);
				// b32 xButton = (gamePad->wButtons & XINPUT_GAMEPAD_X);
				// b32 yButton = (gamePad->wButtons & XINPUT_GAMEPAD_Y);
			}
			else
			{
				newGamePadController->isConnected = false;
			}
		}

		// Fill game services
		GameScreenBuffer gameScreenBuffer = {};
		gameScreenBuffer.height = Win32::internalBuffer.height;
		gameScreenBuffer.width = Win32::internalBuffer.width;
		gameScreenBuffer.pitch = Win32::internalBuffer.pitch;
		gameScreenBuffer.memory = Win32::internalBuffer.memory;

		GameSoundOutput gameSoundBuffer = {};

		// Update
		gameFullUpdate(&gameMemory, &gameScreenBuffer, &gameSoundBuffer, newInputs);

		GameAssert(maxAudioSources >= gameSoundBuffer.maxSoundAssets && "Too much audio sources to handle!");
		GameAssert(maxActiveSounds >= gameSoundBuffer.maxSoundAssets && "Too much audio sources to handle!");

		Win32::UpdateWindow(deviceContext, window, &Win32::internalBuffer);

		//TODO: NOT FINAL AUDIO SYSTEM!
		//? Main assumption(case) is that audio assets are not changed often and iterate all possible IDs (same as array index)
		if (gameSoundBuffer.areNewSoundAssetsAdded)
		{
			for (s32 i = 0; i < gameSoundBuffer.maxSoundAssets; ++i)
			{
				auto &&[wfx, wavData, wavDataSize] = Win32::parseWaveData(gameSoundBuffer.buffer[i].data);

				xaudioCustomBuffers[i].buffer.AudioBytes = wavDataSize;
				xaudioCustomBuffers[i].buffer.pAudioData = wavData;
				xaudioCustomBuffers[i].buffer.Flags = XAUDIO2_END_OF_STREAM;
				xaudioCustomBuffers[i].buffer.LoopCount = gameSoundBuffer.soundsPlayInfos[i].isRepeating ? XAUDIO2_LOOP_INFINITE : 0;
				xaudioCustomBuffers[i].wfx = wfx;
			}
			//TODO: Need to remove old resources manually or create RAII wrapper otherwise it is leaking!
			// Same wav properties assumed for all sound files! 
			for (s32 i = 0; i < maxActiveSounds; i++)
			{
				hr = XAudio2->CreateSourceVoice(&sourceVoices[i], (WAVEFORMATEX *)xaudioCustomBuffers[0].wfx);
				GameAssert((HRESULT)hr >= 0);
			}
		}
		// Process all sounds
		//TODO: For now index '0' is assumed to be for looped menu sound
		for (s32 p = 0; p < gameSoundBuffer.maxSoundAssets; ++p)
		{
			// Fixed ring buffer for dynamic sourcevoices
			while(gameSoundBuffer.soundsPlayInfos[p].count > 0)
			{
				hr = sourceVoices[nextFreeVoiceID]->SubmitSourceBuffer(&xaudioCustomBuffers[p].buffer);
				GameAssert((HRESULT)hr >= 0);
				hr = sourceVoices[nextFreeVoiceID]->Start();
				GameAssert((HRESULT)hr >= 0);

				--gameSoundBuffer.soundsPlayInfos[p].count;
				//? index '0' is reserved for looped
				nextFreeVoiceID = max(1, ( (nextFreeVoiceID + 1) % maxActiveSounds));
			}
		}
		pMasterVoice->SetVolume(gameSoundBuffer.masterVolume);

		// Timers
		cycleEnd = __rdtsc();
		swap(oldInputs, newInputs);

		f32 frameTimeMs = ElapsedMsHere(startFrameCounter.QuadPart);
		if(frameTimeMs < targetFrequencyRate)
		{
			Sleep((DWORD)(targetFrequencyRate - frameTimeMs));
		}
		else
		{
			//TODO: Log or loop time governing
		}

		frameTimeMs =  ElapsedMsHere(startFrameCounter.QuadPart);
		QueryPerformanceCounter(&startFrameCounter);
#if 1
		char tbuffer[32];
		sprintf(tbuffer, "Ms: %.02f\n", frameTimeMs);
		OutputDebugStringA(tbuffer);
#endif
	}
	UnregisterClassA("Scratcher", GetModuleHandleA(nullptr)); // ? Do we need that?
	XAudio2->Release();
}