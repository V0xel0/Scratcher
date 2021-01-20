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
	platform->soundsPlayInfos = gameOut->soundsPlayInfos;
	platform->masterVolume = gameOut->masterVolume;
}

void gameFullUpdate(GameMemory *memory, GameScreenBuffer *buffer, GameSoundOutput *sounds, GameInput *inputs)
{
	GameState *gameState = (GameState *)memory->PermanentStorage;

	//TODO: Should Memory for sound  be from platform? - same as it already is with framebuffer
	GameSoundOutput *soundOutput = (GameSoundOutput *)memory->TransientStorage;
	soundOutput->buffer = (GameSoundAsset *)((byte *)memory->TransientStorage + sizeof(GameSoundOutput));
	soundOutput->soundsPlayInfos = (GameSoundPlayInfo *)((byte *)memory->TransientStorage + 2 * sizeof(GameSoundAsset) + sizeof(GameSoundOutput));

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
		++soundOutput->soundsPlayInfos[Menu1].count;
		soundOutput->soundsPlayInfos[Menu1].isRepeating = true;
		soundOutput->soundsPlayInfos[LaserBullet].isRepeating = false;
		soundOutput->masterVolume = 0.1f;

		memory->isInitialized = true;
	}

	for (s32 controllerID = 0; controllerID < ArrayCount32(inputs->controllers); ++controllerID)
	{
		GameController *controller = getGameController(inputs, controllerID);

		// Analog input processing
		if (controller->isGamePad)
		{
			gameState->colorOffsetX += (s32)(controller->gamePad.StickAverageX*4);
			gameState->colorOffsetY -= (s32)(controller->gamePad.StickAverageY*4);
		}
		else
		{
			gameState->colorOffsetX += controller->mouse.deltaX;
			gameState->colorOffsetY += controller->mouse.deltaY;
			gameState->colorOffsetX += controller->mouse.deltaWheel;
		}
		
		// Digital input processing
		if (controller->moveUp.wasDown && controller->moveUp.halfTransCount)
		{
			soundOutput->masterVolume = clamp(soundOutput->masterVolume + 0.015f, 0.0f, 1.0f);
		}
		if (controller->moveDown.wasDown && controller->moveDown.halfTransCount)
		{
			soundOutput->masterVolume = clamp(soundOutput->masterVolume - 0.015f, 0.0f, 1.0f);
		}
		if (controller->moveLeft.wasDown)
		{
			gameState->colorOffsetX -= 1;
		}
		if (controller->moveRight.halfTransCount)
		{
			gameState->colorOffsetX += 100;
		}
		if (controller->actionFire.wasDown )
		{
			++soundOutput->soundsPlayInfos[LaserBullet].count;
		}
	}

	gameSendSoundsToPlay(sounds, soundOutput);
	gameRender(buffer, gameState->colorOffsetX, gameState->colorOffsetY);

	soundOutput->areNewSoundAssetsAdded = false;
}