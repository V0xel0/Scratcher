#define _CRT_SECURE_NO_WARNINGS

#include "GameAssertions.hpp"
#include "Utils.hpp"
#include "Win32Platform.hpp"
#include "GameService.hpp"

#if UNITY_BUILD
#include "GameService.cpp"
#endif

#include <omp.h>
#include <cstdio>

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

	//TODO: Consider passing it to game?
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

		GameSoundOutput gameSoundBuffer = {};

		// Update
		gameFullUpdate(&gameMemory, &gameBuffer, &gameSoundBuffer);
		Win32::UpdateWindow(deviceContext, window, &Win32::internalBuffer);

		//TODO: NOT FINAL AUDIO SYSTEM!
		//? Main assumption(case) is that audio assets are not changed often and iterate all possible IDs (same as array index)
		if (gameSoundBuffer.areNewSoundAssetsAdded)
		{
			//? Hacky way to check if we replacing existing data
			if (xaudioCustomBuffers[0].wfx != nullptr)
			{
				for (int j = 0; j < maxActiveSounds; ++j)
				{
					hr = sourceVoices[j]->Stop();
					GameAssert((HRESULT)hr >= 0);
					hr = sourceVoices[j]->FlushSourceBuffers();
					GameAssert((HRESULT)hr >= 0);
				}
			}

			for (int i = 0; i < gameSoundBuffer.maxSoundAssets; ++i)
			{
				auto &&[wfx, wavData, wavDataSize] = Win32::parseWaveData(gameSoundBuffer.buffer[i].data);

				xaudioCustomBuffers[i].buffer.AudioBytes = wavDataSize;		 //size of the audio buffer in bytes
				xaudioCustomBuffers[i].buffer.pAudioData = wavData;			 //buffer containing audio data
				xaudioCustomBuffers[i].buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer
				xaudioCustomBuffers[i].buffer.LoopCount = gameSoundBuffer.soundsPlayInfos[i].isRepeating ? XAUDIO2_LOOP_INFINITE : 0;
				xaudioCustomBuffers[i].wfx = wfx;
			}
			//? If same format, frequency, channels for all sound data

			for (int j = 0; j < maxActiveSounds; ++j)
			{
				hr = XAudio2->CreateSourceVoice(&sourceVoices[j], (WAVEFORMATEX *)xaudioCustomBuffers[0].wfx);
				GameAssert((HRESULT)hr >= 0);
			}
		}

		for (int p = 0; p < gameSoundBuffer.maxSoundAssets; ++p)
		{
			bool hack = true;
			for (int s = 0; s < gameSoundBuffer.soundsPlayInfos[p].count; ++s)
			{
				//TODO: Check if it is possible to skip "CreateSourceVoice" - do it only once
				if (hack)
				{
					hr = XAudio2->CreateSourceVoice(&sourceVoices[nextFreeVoiceID], (WAVEFORMATEX *)xaudioCustomBuffers[p].wfx);
					GameAssert((HRESULT)hr >= 0);
					hack = !hack;
				}

				hr = sourceVoices[nextFreeVoiceID]->SubmitSourceBuffer(&xaudioCustomBuffers[p].buffer);
				GameAssert((HRESULT)hr >= 0);
				hr = sourceVoices[nextFreeVoiceID]->Start();
				GameAssert((HRESULT)hr >= 0);

				gameSoundBuffer.soundsPlayInfos[p].count = gameSoundBuffer.soundsPlayInfos[p].count > 0 ? --gameSoundBuffer.soundsPlayInfos[p].count : 0;
				nextFreeVoiceID = (nextFreeVoiceID + 1) % maxActiveSounds;
			}
		}
		pMasterVoice->SetVolume(gameSoundBuffer.masterVolume);

		// Timers
		s64 frameTimeMs = ElapsedMsHere(startTime.QuadPart);
		cycleEnd = __rdtsc();
		u64 mcpf = (cycleStart - cycleEnd) / (1'000'000);

		//TODO: Temporary for debug
		char tbuffer[32];
		sprintf(tbuffer, "Ms: %lld\n", frameTimeMs);
		OutputDebugStringA(tbuffer);
	}
	UnregisterClassA("Scratcher", GetModuleHandleA(nullptr)); // ? Do we need that?
	CoUninitialize();
}