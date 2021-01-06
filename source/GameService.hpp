#pragma once

// This header declares services and data which game then provides to platform layer
// Architecture style inspired from "handmadehero" series - Game is treated as a service to OS, instead of 
// abstracting platform code and handling it as kind of "Virtual OS"

struct GameScreenBuffer
{
	void *memory;
	s32 width;
	s32 height;
	s32 pitch;
};

struct GameInputPad
{

};

struct GameInputMouse
{

};

struct GameSoundAsset
{
	void *data;
	u64 size;
};

//? Cause of XAudio2 design, game is providing memory, this breaks base architecture style for audio but...
struct GameSoundOutput
{
	s32 maxSoundAssets;
	b32 areNewSoundAssetsAdded;
	s32 *soundsPlayingCounts;
	GameSoundAsset *buffer;
};

struct GameKeyState
{
	s32 halfTransCount;
	b32 wasDown;
};

struct GameInput
{

};

struct GameMemory
{
	b32 isInitialized;
	u64 PermanentStorageSize;
	void *PermanentStorage;

	u64 TransientStorageSize;
	void *TransientStorage;
};

struct GameState
{
	s32 colorOffsetX;
	s32 colorOffsetY;
};

void gameFullUpdate(GameMemory *memory, GameScreenBuffer *buffer, GameSoundOutput *sounds);