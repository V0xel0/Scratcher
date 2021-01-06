#include "GameService.hpp"
#include "Utils.hpp"

enum SoundTypeID
{
	Menu1,
	LaserBullet,
	CountOfTypes
};

// TEST-ONLY-FUNCTION for checking basic pixel drawing & looping
internal void gameRender(GameScreenBuffer *gameBuffer, const s32 offsetX, const s32 offsetY)
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

internal void gameSendSoundsToPlay(GameSoundOutput *platform, GameSoundOutput *gameOut)
{
	platform->areNewSoundAssetsAdded = gameOut->areNewSoundAssetsAdded;
	platform->buffer = gameOut->buffer;
	platform->maxSoundAssets = gameOut->maxSoundAssets;
	platform->soundsPlayingCounts = gameOut->soundsPlayingCounts;
}

void gameFullUpdate(GameMemory *memory, GameScreenBuffer *buffer, GameSoundOutput *sounds)
{
	GameState *gameState = (GameState *)memory->PermanentStorage;

	GameSoundOutput *soundOutput = (GameSoundOutput *)memory->TransientStorage;
	soundOutput->buffer = (GameSoundAsset *)((byte *)memory->TransientStorage + sizeof(GameSoundOutput));
	soundOutput->soundsPlayingCounts = (s32 *)((byte *)memory->TransientStorage + 2 * sizeof(GameSoundAsset) + sizeof(GameSoundOutput));

	if (!memory->isInitialized)
	{
		gameState->colorOffsetX = 0;
		gameState->colorOffsetY = 0;

		//TODO: All sound loading by "DebugReadFile" is temporary!
		auto &&[rawFileData, rawFileSize] = Win32::DebugReadFile("D:/menu_1.wav");
		soundOutput->buffer[Menu1].data = rawFileData;
		soundOutput->buffer[Menu1].size = rawFileSize;

		auto &&[otherFileData, otherFileSize] = Win32::DebugReadFile("D:/laser_1.wav");
		soundOutput->buffer[LaserBullet].data = otherFileData;
		soundOutput->buffer[LaserBullet].size = otherFileSize;

		soundOutput->maxSoundAssets = 2;
		soundOutput->areNewSoundAssetsAdded = true;
		//TODO: Test only
		++soundOutput->soundsPlayingCounts[Menu1];
		++soundOutput->soundsPlayingCounts[LaserBullet];


		memory->isInitialized = true;
	}

	gameSendSoundsToPlay(sounds, soundOutput);
	gameRender(buffer, gameState->colorOffsetX, gameState->colorOffsetY);

	soundOutput->areNewSoundAssetsAdded = false;
}