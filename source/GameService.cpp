#include "GameService.hpp"
#include "Utils.hpp"

// TEST-ONLY-FUNCTION for checking basic pixel drawing & looping
internal void testRender(GameScreenBuffer *gameBuffer, const s32 offsetX, const s32 offsetY)
{
	s32 width = gameBuffer->width;
	s32 height = gameBuffer->height;

	u8 *row = (u8 *)gameBuffer->memory;
#if 1
	for (s32 y = 0; y < height; y++)
	{
		u32 *pixel = (u32 *)row;
		for (s32 x = 0; x < width; x++)
		{
			u8 b = (u8)(x + offsetX);
			u8 g = (u8)(y + offsetY);
			u8 r = 255;

			*pixel++ = ((r << 16) | (g << 8) | b);
		}
		row += gameBuffer->pitch;
	}
#endif
	//? Multithreaded, but single core is slower cause of division and modulo
#if 0
	omp_set_num_threads(12);
#pragma omp parallel for
	for (int xy = 0; xy < width*height; ++xy) 
	{
		s32 x = xy % width;
		s32 y = xy / width;
		
		u32 *pixel = ( (u32*)row ) + xy;
		u8 b = (u8)(x + offsetX);
		u8 g = (u8)(y + offsetY);
		u8 r = (u8)Win32::mouseData.x;
		*pixel = ((r << 16) |(g << 8) | b);
	}
#endif
}

void gameFullUpdate(GameMemory *memory, GameScreenBuffer *buffer, GameOutputSound *sounds)
{
	GameState *gameState = (GameState *)memory->PermanentStorage;
	GameSound *soundToPlayBuffer = (GameSound *)memory->TransientStorage;
	s32 *soundsToPlay = (s32 *)((byte *)memory->TransientStorage + 2 * sizeof(GameSound));

	if (!memory->isInitialized)
	{
		gameState->colorOffsetX = 0;
		gameState->colorOffsetY = 0;

		//TODO: All sound playing by "DebugReadFile" is temporary!
		auto &&[rawFileData, rawFileSize] = Win32::DebugReadFile("D:/menu_1.wav");
		soundToPlayBuffer->data = rawFileData;
		soundToPlayBuffer->size = rawFileSize;

		auto &&[otherFileData, otherFileSize] = Win32::DebugReadFile("D:/laser_1.wav");
		soundToPlayBuffer[1].data = otherFileData;
		soundToPlayBuffer[1].size = otherFileSize;

		sounds->areNewSoundAssetsAdded = true;
		sounds->maxSoundSources = 2;
		sounds->buffer = soundToPlayBuffer;
		sounds->playCounts = soundsToPlay;

		++sounds->playCounts[0];
		++sounds->playCounts[1];

		memory->isInitialized = true;
	}

	sounds->maxSoundSources = 2;
	sounds->buffer = soundToPlayBuffer;
	sounds->playCounts = soundsToPlay;

	testRender(buffer, gameState->colorOffsetX, gameState->colorOffsetY);
}