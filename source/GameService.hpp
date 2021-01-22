#pragma once

#include "Utils.hpp"

// This header declares services and data which game then provides to platform layer
// Architecture style inspired from "handmadehero" series - Game is treated as a service to OS, instead of
// abstracting platform code and handling it as kind of "Virtual OS"

#if GAME_INTERNAL
	struct DebugFileOutput
	{
		void *data;
		u32 dataSize;
	};
#endif
struct GameScreenBuffer
{
	void *memory;
	s32 width;
	s32 height;
	s32 pitch;
};

struct GameSoundAsset
{
	void *data;
	u64 size;
};

struct GameSoundPlayInfo
{
	s16 count;
	b16 isRepeating;
};

struct GameSoundOutput
{
	s32 maxSoundAssets;
	b32 areNewSoundAssetsAdded;
	f32 masterVolume;

	GameSoundPlayInfo *soundsPlayInfos;
	GameSoundAsset *buffer;
};

struct GameKeyState
{
	s32 halfTransCount;
	b32 wasDown;
};

struct GameMouseData
{
	s32 x;
	s32 y;
	s32 deltaX;
	s32 deltaY;
	s32 deltaWheel;
};

struct GameGamePadData
{
	f32 StickAverageX;
	f32 StickAverageY;
};

struct GameController
{
	b16 isGamePad;
	b16 isConnected;
	union
	{
		GameMouseData mouse;
		GameGamePadData gamePad;
	};

	union
	{
		GameKeyState buttons[12];
		struct
		{
			GameKeyState moveUp;
			GameKeyState moveDown;
			GameKeyState moveLeft;
			GameKeyState moveRight;

			GameKeyState actionFire;
			GameKeyState action1;
			GameKeyState action2;
			GameKeyState action3;
			GameKeyState action4;
			GameKeyState action5;

			GameKeyState back;
			GameKeyState start;

			// All buttons must be added above this line

			GameKeyState terminator;
		};
	};
};

struct GameInput
{
	//TODO: Explicitly message number of players
	GameController controllers[2];
};

inline GameController *getGameController(GameInput *input, u32 controllerID)
{
    GameAssert(controllerID < (u32)ArrayCount64(input->controllers));
    return &input->controllers[controllerID];
}

struct GameMemory
{
	b32 isInitialized;
	u64 PermanentStorageSize;
	void *PermanentStorage;

	u64 TransientStorageSize;
	void *TransientStorage;

#if GAME_INTERNAL
	void (*DEBUGplatformFreeFile)(void *);
    DebugFileOutput (*DEBUGPlatformReadFile)(char *);
    b32 (*DEBUGPlatformWriteFile)(char *, void *, u32);
#endif
};

struct GameState
{
	s32 colorOffsetX;
	s32 colorOffsetY;
};

#define GAME_FULL_UPDATE(name) void name(GameMemory *memory, GameScreenBuffer *buffer, GameSoundOutput *sounds, GameInput *inputs)
typedef GAME_FULL_UPDATE(gameFullUpdatePtr);
GAME_FULL_UPDATE(gameFullUpdateNotLoaded)
{
}