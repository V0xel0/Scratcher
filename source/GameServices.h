#pragma once
#include "types.h"

// Services and data that game need from platform layer

struct GameScreenBuffer
{
	void *memory;
	s32 width;
	s32 height;
	s32 pitch;
};

void GameFullUpdate(GameScreenBuffer *buffer, const s32 colorOffsetX, const s32 colorOffsetY);