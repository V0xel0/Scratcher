#include "Utils.hpp"
#include "GameMath.hpp"
#include "Allocators.hpp"
#include "GameService.hpp"
#include "Game.hpp"
#include <cmath>
#include <cstring>

enum SoundTypeID
{
	Menu1,
	LaserBullet,
	CountOfTypes
};

//TODO: Temporary
global_variable constexpr s32 worldWidth = 24;
global_variable constexpr s32 worldHeight = 24;
global_variable byte worldMap[worldWidth][worldHeight]
{
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,2,2,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,3,0,0,0,3,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,2,0,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,0,0,0,5,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

//TODO: TEST-ONLY, function for checking basic pixel drawing & looping
internal void debugDrawGradient(const GameScreenBuffer *gameBuffer, const s32 offsetX, const s32 offsetY)
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
	for (s32 xy = 0; xy < width*height; ++xy) 
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

//TODO: Not final
internal void drawRectangle(const GameScreenBuffer *gameBuffer, Vec2 start, Vec2 end, u32 color = 0xFF'FF'FF'FF)
{
	using std::round;

	s32 minX =  max((s32)round(start.x), 0);
	s32 minY =  max((s32)round(start.y), 0);
	s32 maxX =  min((s32)round(end.x), gameBuffer->width);
	s32 maxY =  min((s32)round(end.y), gameBuffer->height);

	byte *row = ((byte *)gameBuffer->memory + minX*gameBuffer->bytesPerPixel + minY*gameBuffer->pitch);

	for (s32 y = minY; y < maxY; y++)
	{
		u32 *pixel = (u32*)row;
		for (s32 x = minX; x < maxX; x++)
		{
			*pixel++ = color;
		}
		row += gameBuffer->pitch;
	}
}

internal void drawCrossHair(const GameScreenBuffer *gameBuffer, u32 color = 0xFF'FF'FF'FF)
{
	//TODO: TEST-ONLY, later bitmap or smth more efficient!
	drawRectangle(gameBuffer, {(f32)gameBuffer->width/2-8, (f32)gameBuffer->height/2-2}, {(f32)gameBuffer->width/2+8, (f32)gameBuffer->height/2+2});
	drawRectangle(gameBuffer, {(f32)gameBuffer->width/2-2, (f32)gameBuffer->height/2-8}, {(f32)gameBuffer->width/2+2, (f32)gameBuffer->height/2+8});
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
		gameState->playerPosition.x = 22;
		gameState->playerPosition.y = 12;
		gameState->playerDirection = {-1.0f, 0.0f};
		gameState->projectionPlane = {0.0f, 0.66f};

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
				//TODO: Gamepad support
				//gameState->playerPosition.x += (s32)(controller.gamePad.StickAverageX*4);
				//gameState->playerPosition.y -= (s32)(controller.gamePad.StickAverageY*4);
			}
			else
			{
				f32 rotateSpeed = -(f32)controller.mouse.deltaX * 0.025f;

				f32 oldDirX = gameState->playerDirection.x;
				gameState->playerDirection.x = gameState->playerDirection.x * cos(rotateSpeed) - gameState->playerDirection.y * sin(rotateSpeed);
				gameState->playerDirection.y = oldDirX * sin(rotateSpeed) + gameState->playerDirection.y * cos(rotateSpeed);

				f32 oldPlaneX = gameState->projectionPlane.x;
				gameState->projectionPlane.x = gameState->projectionPlane.x * cos(rotateSpeed) - gameState->projectionPlane.y * sin(rotateSpeed);
				gameState->projectionPlane.y = oldPlaneX * sin(rotateSpeed) + gameState->projectionPlane.y * cos(rotateSpeed);
			}
			
			// Digital input processing
			if (controller.moveUp.wasDown )
			{
				f32 moveSpeed = 0.5f;
				if(worldMap[s32(gameState->playerPosition.x + gameState->playerDirection.x * moveSpeed)][s32(gameState->playerPosition.y)] == 0) 
					gameState->playerPosition.x += gameState->playerDirection.x * moveSpeed;
      			if(worldMap[s32(gameState->playerPosition.x)][s32(gameState->playerPosition.y + gameState->playerDirection.y * moveSpeed)] == 0)
					gameState->playerPosition.y +=  gameState->playerDirection.y * moveSpeed;
			}
			if (controller.moveDown.wasDown)
			{
				f32 moveSpeed = 0.5f;
				if(worldMap[s32(gameState->playerPosition.x - gameState->playerDirection.x * moveSpeed)][s32(gameState->playerPosition.y)] == 0) 
					gameState->playerPosition.x -= gameState->playerDirection.x * moveSpeed;
      			if(worldMap[s32(gameState->playerPosition.x)][s32(gameState->playerPosition.y - gameState->playerDirection.y * moveSpeed)] == 0)
					gameState->playerPosition.y -=  gameState->playerDirection.y * moveSpeed;
			}
			if (controller.moveLeft.wasDown)
			{
				
			}
			if (controller.moveRight.wasDown)
			{
				
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
		//gameState->playerPosition.y += (s32)(6.0f*sinf(0.5f*PI32*gameState->gravityJump));
	}

	gameState->gravityJump -= 0.045f;
	//TODO: change to movaps, movsb or other SSE
	memset(buffer->memory, 0, buffer->height*buffer->width*buffer->bytesPerPixel);
	
	{
		//TODO: Refactor, understand and optimize all in scope
		f32 posX = gameState->playerPosition.x, posY = gameState->playerPosition.y;  //x and y start position
		f32 dirX = gameState->playerDirection.x, dirY = gameState->playerDirection.y; //initial direction vector
		f32 planeX = gameState->projectionPlane.x, planeY = gameState->projectionPlane.y; //the 2d raycaster version of camera plane

		for(s32 x = 0; x < buffer->width; x++)
		{
			//calculate ray position and direction
			f32 cameraX = 2 * x / (f32)buffer->width - 1; //x-coordinate in camera space
			f32 rayDirX = dirX + planeX * cameraX;
			f32 rayDirY = dirY + planeY * cameraX;
			//which box of the map we're in
			s32 mapX = s32(posX);
			s32 mapY = s32(posY);

			//length of ray from current position to next x or y-side
			f32 sideDistX;
			f32 sideDistY;

			//length of ray from one x or y-side to next x or y-side
			f32 deltaDistX = std::abs(1 / rayDirX);
			f32 deltaDistY = std::abs(1 / rayDirY);
			f32 perpWallDist;

			//what direction to step in x or y-direction (either +1 or -1)
			s32 stepX;
			s32 stepY;

			s32 hit = 0; //was there a wall hit?
			s32 side; //was a NS or a EW wall hit?
			//calculate step and initial sideDist
			if(rayDirX < 0)
			{
				stepX = -1;
				sideDistX = (posX - mapX) * deltaDistX;
			}
			else
			{
				stepX = 1;
				sideDistX = (mapX + 1.0f - posX) * deltaDistX;
			}
			if(rayDirY < 0)
			{
				stepY = -1;
				sideDistY = (posY - mapY) * deltaDistY;
			}
			else
			{
				stepY = 1;
				sideDistY = (mapY + 1.0f - posY) * deltaDistY;
			}
			//perform DDA
			while (hit == 0)
			{
				//jump to next map square, OR in x-direction, OR in y-direction
				if(sideDistX < sideDistY)
				{
				sideDistX += deltaDistX;
				mapX += stepX;
				side = 0;
				}
				else
				{
				sideDistY += deltaDistY;
				mapY += stepY;
				side = 1;
				}
				//Check if ray has hit a wall
				if(worldMap[mapX][mapY] > 0) hit = 1;
			}
			//Calculate distance projected on camera direction (Euclidean distance will give fisheye effect!)
			if(side == 0) perpWallDist = (mapX - posX + (1 - stepX) / 2) / rayDirX;
			else          perpWallDist = (mapY - posY + (1 - stepY) / 2) / rayDirY;

			//Calculate height of line to draw on screen
			s32 lineHeight = (s32)(buffer->height / perpWallDist);

			//calculate lowest and highest pixel to fill in current stripe
			s32 drawStart = -lineHeight / 2 + buffer->height / 2;
			if(drawStart < 0)drawStart = 0;
			s32 drawEnd = lineHeight / 2 + buffer->height / 2;
			if(drawEnd >= buffer->height)drawEnd = buffer->height - 1;

			//choose wall color
			u32 color;
			switch(worldMap[mapX][mapY])
			{
				case 1:  color = (u32)0xFF'FF'00'00;	break; //red
				case 2:  color = (u32)0x00'00'F0'00;	break; //green
				case 3:  color = (u32)0x00'00'00'FF;	break; //blue
				case 4:  color = (u32)0x0F'0F'FF'FF;	break; //white
				default: color = (u32)0xFF'FF'FF'FF;	break; //yellow
			}

			//give x and y sides different brightness
			if(side == 1) {color = color / 2;}

			//draw the pixels of the stripe as a vertical line
			drawRectangle(buffer, {(f32)x, (f32)drawStart}, {(f32)x+1, (f32)drawEnd}, color);
		}
	}
	drawCrossHair(buffer);
}