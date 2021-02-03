#include "Utils.hpp"
#include "Allocators.hpp"
#include "GameService.hpp"
#include "Game.hpp"
#include "cmath"

enum SoundTypeID
{
	Menu1,
	LaserBullet,
	CountOfTypes
};

//TODO: TEST-ONLY, function for checking basic pixel drawing & looping
internal void gameRender(const GameScreenBuffer *gameBuffer, const s32 offsetX, const s32 offsetY)
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

//TODO: TEST-ONLY TEMPORARY function
internal void playerRender(const GameScreenBuffer *gameBuffer, const s32 playerX,  const s32 playerY)
{
	byte *bufferEnd = (byte *)gameBuffer->memory + gameBuffer->pitch * gameBuffer->height;

	u32 color = 0xFF'FF'FF'FF;
	s32 top = playerY;
	s32 bottom = playerY+10;
	for (s32 x = playerX; x < playerX+10; x++)
	{
		byte *pixel = ((byte *)gameBuffer->memory + x * gameBuffer->bytesPerPixel + top * gameBuffer->pitch);
		for (s32 y = playerY; y < bottom; y++)
		{
			if( (pixel >= gameBuffer->memory) && (pixel + 4) <= bufferEnd)
			{
				*(u32 *)pixel = color;
			}
			pixel += gameBuffer->pitch;
		}
	}
}

extern "C" GAME_FULL_UPDATE(gameFullUpdate)
{
	GameState *gameState = (GameState *)memory->PermanentStorage;

	if (!memory->isInitialized)
	{
		gameState->colorOffsetX = 0;
		gameState->colorOffsetY = 0;

		arenaInit(&gameState->worldStorage, memory->PermanentStorageSize - sizeof(gameState), (byte *)memory->PermanentStorage + sizeof(GameState) );
		arenaInit(&gameState->assetsStorage, memory->TransientStorageSize, (byte*)memory->TransientStorage);

		//TODO: TEST-ONLY, All sound loading by "debugReadFile" is temporary!
		gameState->soundsAssetCount = 2;
		gameState->soundsBuffer = arenaPush<GameSoundAsset>(&gameState->assetsStorage, gameState->soundsAssetCount);
		gameState->soundInfos = arenaPush<GameSoundPlayInfo>(&gameState->assetsStorage, gameState->soundsAssetCount);

		auto &&[rawFileData, rawFileSize] = memory->DEBUGPlatformReadFile("../assets/menu_1.wav");
		gameState->soundsBuffer[Menu1].data = rawFileData;
		gameState->soundsBuffer[Menu1].size = rawFileSize;

		auto &&[otherFileData, otherFileSize] = memory->DEBUGPlatformReadFile("../assets/laser_1.wav");
		gameState->soundsBuffer[LaserBullet].data = otherFileData;
		gameState->soundsBuffer[LaserBullet].size = otherFileSize;

		sounds->soundsPlayInfos = gameState->soundInfos;

		//TODO: TEST-ONLY
		sounds->areNewSoundAssetsAdded = true;
		++sounds->soundsPlayInfos[Menu1].count;
		sounds->soundsPlayInfos[Menu1].isRepeating = true;
		sounds->soundsPlayInfos[LaserBullet].isRepeating = false;

		//TODO: TEST-ONLY, SOME TEMPORARY LOGIC
		gameState->playerX = 100;
		gameState->playerY = 100;

		memory->isInitialized = true;
	}

	// Fill platform audio buffer
	sounds->buffer = gameState->soundsBuffer;
	sounds->soundsPlayInfos = gameState->soundInfos;
	sounds->maxSoundAssets = gameState->soundsAssetCount;
	sounds->masterVolume = 0.1f;

	for (auto& controller : inputs->controllers)
	{
		if(controller.isConnected)
		{
			// Analog input processing
			if (controller.isGamePad)
			{
				// gameState->colorOffsetX += (s32)(controller.gamePad.StickAverageX*4);
				// gameState->colorOffsetY -= (s32)(controller.gamePad.StickAverageY*4);
				gameState->playerX += (s32)(controller.gamePad.StickAverageX*4);
				gameState->playerY -= (s32)(controller.gamePad.StickAverageY*4);
			}
			else
			{
				gameState->colorOffsetX += controller.mouse.deltaX;
				gameState->colorOffsetY += controller.mouse.deltaY;

				gameState->colorOffsetX += controller.mouse.deltaWheel;

				gameState->playerX = controller.mouse.x;
				gameState->playerY = controller.mouse.y;
			}
			
			// Digital input processing
			if (controller.moveUp.wasDown && controller.moveUp.halfTransCount)
			{
				sounds->masterVolume = clamp(sounds->masterVolume + 0.015f, 0.0f, 1.0f);
			}
			if (controller.moveDown.wasDown && controller.moveDown.halfTransCount)
			{
				sounds->masterVolume = clamp(sounds->masterVolume - 0.015f, 0.0f, 1.0f);
			}
			if (controller.moveLeft.wasDown)
			{
				gameState->colorOffsetX -= 50;
			}
			if (controller.moveRight.halfTransCount)
			{
				gameState->colorOffsetX += 100;
			}
			if (controller.actionFire.wasDown && controller.actionFire.halfTransCount)
			{
				++sounds->soundsPlayInfos[LaserBullet].count;
				gameState->gravityJump = 4.0f;
			}
		}
		else
		{
			//TODO: LOG
		}
	}

	//TODO: TEST-ONLY, SOME TEMPORARY LOGIC
	if(gameState->gravityJump > 0)
	{
		gameState->playerY += (s32)(6.0f*sinf(0.5f*PI32*gameState->gravityJump));
	}
	gameState->gravityJump -= 0.045f;

	gameRender(buffer, gameState->colorOffsetX, gameState->colorOffsetY);
	playerRender(buffer, gameState->playerX, gameState->playerY);
}