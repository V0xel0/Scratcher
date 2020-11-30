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

struct GameSoundSystem
{

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

void GameFullUpdate(GameMemory *memory, GameScreenBuffer *buffer);