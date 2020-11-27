#define _CRT_SECURE_NO_WARNINGS

#include "Utils.h"
#include "GameServices.h"

#if UNITY_BUILD
#include "GameServices.cpp"
#endif

#include "Win32Platform.h"
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

	GameScreenBuffer gameBuffer = {};
	gameBuffer.height =  Win32::internalBuffer.height;
	gameBuffer.width  =  Win32::internalBuffer.width;
	gameBuffer.pitch  =  Win32::internalBuffer.pitch;
	gameBuffer.memory =  Win32::internalBuffer.memory;

	// Audio stuff

	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	Microsoft::WRL::ComPtr<IXAudio2> XAudio2;
	HRESULT hr;
	hr = XAudio2Create( &XAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR );
	assert( (HRESULT) hr >= 0 );
	IXAudio2MasteringVoice *pMasterVoice = nullptr;
	hr = XAudio2->CreateMasteringVoice( &pMasterVoice );
	assert( (HRESULT) hr >= 0);

	WAVEFORMATEXTENSIBLE wfx = {};
	XAUDIO2_BUFFER xaudioBuffer = {};
	
	WCHAR * strFileName = (L"D:\\POL-guilty-one-short.wav");

	// Open the file
	HANDLE hFile = CreateFileW(
		strFileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL );

	// if( INVALID_HANDLE_VALUE == hFile )
	// 	return HRESULT_FROM_WIN32( GetLastError() );

	// if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, 0, NULL, FILE_BEGIN ) )
	// 	return HRESULT_FROM_WIN32( GetLastError() );

	DWORD dwChunkSize;
	DWORD dwChunkPosition;
	//check the file type, should be fourccWAVE or 'XWMA'
	Win32::FindChunk(hFile,fourccRIFF,dwChunkSize, dwChunkPosition );
	DWORD filetype;
	Win32::ReadChunkData(hFile,&filetype,sizeof(DWORD),dwChunkPosition);
	// if (filetype != fourccWAVE)
	// 	return S_FALSE;

	Win32::FindChunk(hFile,fourccFMT, dwChunkSize, dwChunkPosition );
	Win32::ReadChunkData(hFile, &wfx, dwChunkSize, dwChunkPosition );

	//fill out the audio data buffer with the contents of the fourccDATA chunk
	Win32::FindChunk(hFile,fourccDATA,dwChunkSize, dwChunkPosition );
	BYTE * pDataBuffer = new BYTE[dwChunkSize];
	Win32::ReadChunkData(hFile, pDataBuffer, dwChunkSize, dwChunkPosition);

	xaudioBuffer.AudioBytes = dwChunkSize;  //size of the audio buffer in bytes
	xaudioBuffer.pAudioData = pDataBuffer;  //buffer containing audio data
	xaudioBuffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer
	xaudioBuffer.LoopCount = XAUDIO2_LOOP_INFINITE;

	// Create Source Voice and submit buffer to it
	IXAudio2SourceVoice* pSourceVoice;
	hr = XAudio2->CreateSourceVoice( &pSourceVoice, (WAVEFORMATEX*)&wfx );
	assert( (HRESULT) hr >= 0);
	hr = pSourceVoice->SubmitSourceBuffer( &xaudioBuffer );
    assert( (HRESULT) hr >= 0);

	// Activate Source voice
	hr = pSourceVoice->Start();
   	assert( (HRESULT) hr >= 0);

	// Main Win32 platform loop
	while (isRunning)
	{
		QueryPerformanceCounter(&startTime);
		cycleStart = __rdtsc();
		
		// Windows message dispatching loop
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

		// Update
		GameFullUpdate(&gameBuffer, XOffset, YOffset);
		Win32::UpdateWindow(deviceContext, window, &Win32::internalBuffer);
		
		++XOffset;
		YOffset += 2;

		// Timers
		s64 frameTimeMs = ElapsedMsHere(startTime.QuadPart);
		cycleEnd = __rdtsc();
		u64 mcpf = (cycleStart-cycleEnd) / (1'000'000);

		// Temporary for debug
		char tbuffer[32];
		sprintf(tbuffer, "Ms: %lld\n", frameTimeMs);
		OutputDebugStringA(tbuffer);
	}
	UnregisterClassA("Scratcher", GetModuleHandleA(nullptr)); // ? Do we need that?
	CoUninitialize();
}