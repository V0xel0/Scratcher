#include "Utils.hpp"
#include "Intrinsics.hpp"
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
global_variable byte worldMap[worldHeight][worldWidth]
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

internal b8 checkMapCollision(const Vec2 pos)
{
	b8 out = false;
	out = worldMap[s32(pos.y)][s32(pos.x)] == 0 ? true : false;
	return out;
}

internal Vec2 movePlayer(const Player player, const f32 speedDir, const f32 speedStrife)
{
	Vec2 newPos = player.pos + player.dir * speedDir;
	Vec2 out = player.pos;

	if( checkMapCollision({newPos.x, player.pos.y}) )
	{
		out.x = newPos.x;
	}
	if( checkMapCollision({player.pos.x, newPos.y}) )
	{
		out.y = newPos.y;
	}

	newPos = out + Vec2{-player.dir.y, player.dir.x} * speedStrife;
	if ( checkMapCollision(newPos) )
	{
		out = newPos;
	}
	return out;
}

internal Texture DEBUGLoadTextureFromBMP(DebugFileOutput (*DEBUGPlatformReadFile)(const char *), const char *fileName)
{
	#pragma pack(push, 1)
	struct BmpHeader
	{
		u16 type;
		u32 fileFize;
		u16 reserved1;
		u16 reserved2;
		u32 offset;
		u32 infoSize;
		s32 width;
		s32 height;
		u16 planes;
		u16 bitsPerPixel;
		u32 compression;
		u32 dataSize;
		s32 hRez;
		s32 vRez;
		u32 colors1;
		u32 colors2;
		// Needed for 32bits BMPs
		u32 redMask;
		u32 greenMask;
		u32 blueMask;
	};
	#pragma pack(pop)

	Texture out = {};
	auto &&[fileData, fileSize] = DEBUGPlatformReadFile(fileName);

	if(fileData != nullptr)
	{
		BmpHeader *header = (BmpHeader *)fileData;
		if (header->bitsPerPixel == 32 && header->compression == 3)
		{
			out.pixels = (u32*)((byte *)fileData + header->offset);
			out.height = header->height;
			out.width = header->width;
			out.size = header->dataSize;

			auto redShift = findFirstSetLSB(header->redMask);
			auto greenShift = findFirstSetLSB(header->greenMask);
			auto blueShift = findFirstSetLSB(header->blueMask);
			auto alphaShift = findFirstSetLSB(~(header->redMask | header->greenMask | header->blueMask));

			u32 *src = out.pixels;
			for (s32 y = 0; y < header->height; y++)
			{
				for (s32 x = 0; x < header->width; x++)
				{
					u32 m = *src;
					*src++ = (((  (m >> alphaShift) & 0xFF) << 24)	|
								(((m >> redShift) 	& 0xFF) << 16)	|
								(((m >> greenShift) & 0xFF) << 8)	|
								(((m >> blueShift) 	& 0xFF) << 0));
				}
			}
		}
		else
		{
			//TODO: LOG
			GameAssert(0);
		}
	}
	else
	{
		//TODo: LOG
		GameAssert(0);
	}
	return out;
};	

internal void drawTexture(GameScreenBuffer *gameBuffer, Texture *texture, Vec2 start, s32 alignX = 0, s32 alignY = 0)
{
	using std::round;

	start.x -= (f32)alignX;
	start.y -= (f32)alignY;

	s32 srcOffsetX = 0;
	s32 srcOffsetY = 0;

	if(start.x < 0)
		srcOffsetX = (s32)round(-start.x);
	if(start.y < 0)
		srcOffsetY = (s32)round(-start.y);

	s32 minX =  max((s32)round(start.x), 0);
	s32 minY =  max((s32)round(start.y), 0);
	s32 maxX =  min((s32)round(start.x + (f32)texture->width), gameBuffer->width);
	s32 maxY =  min((s32)round(start.y + (f32)texture->height), gameBuffer->height);

	u32 *srcRow = texture->pixels + texture->width * (texture->height - 1);
	srcRow += -srcOffsetY*texture->width + srcOffsetX;
	u8 *destRow = ((byte *)gameBuffer->memory + minX*gameBuffer->bytesPerPixel + minY*gameBuffer->pitch);

	for (s32 y = minY; y < maxY; y++)
	{
		u32 *dest = (u32 *)destRow;
		u32 *src = srcRow;
		for (s32 x = minX; x < maxX; x++)
		{
			//TODO: Color as vector and premultiply alpha with gamma correction (LUT?)
			f32 a = (f32)((*src >> 24) & 0xFF) / 255.0f;
            f32 sr = (f32)((*src >> 16) & 0xFF);
            f32 sg = (f32)((*src >> 8) & 0xFF);
            f32 sb = (f32)((*src >> 0) & 0xFF);

            f32 dr = (f32)((*dest >> 16) & 0xFF);
            f32 dg = (f32)((*dest >> 8) & 0xFF);
            f32 db = (f32)((*dest >> 0) & 0xFF);

            f32 r = (1.0f-a)*dr + a*sr;
            f32 g = (1.0f-a)*dg + a*sg;
            f32 b = (1.0f-a)*db + a*sb;

            *dest = (((u32)(r + 0.5f) << 16) |
                     ((u32)(g + 0.5f) << 8)	 |
                     ((u32)(b + 0.5f) << 0));
            
            ++dest;
            ++src;
		}
		destRow += gameBuffer->pitch;
        srcRow -= texture->width;
	}
	
}

//TODO: TEST-ONLY, function for checking basic pixel drawing & looping
internal void DEBUGDrawGradient(const GameScreenBuffer *gameBuffer, const s32 offsetX, const s32 offsetY)
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

internal void drawGun(GameScreenBuffer *gameBuffer, GunTexture *gunTexture)
{
	//TODO: Change animation sprite here
	drawTexture(gameBuffer, &gunTexture->anim0, 
			{(f32)gameBuffer->width/2 - (f32)gunTexture->anim0.width/2, 
			gameBuffer->height/2 + (f32)gunTexture->anim0.height/2}, 
			gunTexture->alignX, gunTexture->alignY);
}

extern "C" GAME_FULL_UPDATE(gameFullUpdate)
{
	GameState *gameState = (GameState *)memory->PermanentStorage;

	if (!memory->isInitialized)
	{
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
		gameState->soundMasterVolume = 0.1f;

		//TODO: TEST-ONLY
		sounds->areNewSoundAssetsAdded = true;
		++sounds->soundsPlayInfos[Menu1].count;
		sounds->soundsPlayInfos[Menu1].isRepeating = true;
		sounds->soundsPlayInfos[LaserBullet].isRepeating = false;

		gameState->player.pos.x = 12;
		gameState->player.pos.y = 22;
		gameState->player.dir = {0.0f, -1.0f};
		gameState->projectionPlane = {1.0f, 0.0f};

		// Gun textures
		gameState->gunTextures[rayGun].anim0 = DEBUGLoadTextureFromBMP(memory->DEBUGPlatformReadFile, "../assets/rayGunHD0.bmp");
		gameState->gunTextures[rayGun].alignX = 15;
		gameState->gunTextures[rayGun].alignY = 40;

		gameState->gunTextures[greenGun].anim0 = DEBUGLoadTextureFromBMP(memory->DEBUGPlatformReadFile, "../assets/greenGunHD0.bmp");
		gameState->gunTextures[greenGun].alignX = 0;
		gameState->gunTextures[greenGun].alignY = 150;

		gameState->activeGun = rayGun;

		memory->isInitialized = true;
	}

	for (auto& controller : inputs->controllers)
	{
		if(controller.isConnected)
		{
			// Analog input processing
			if (controller.isGamePad)
			{
				f32 moveSpeed = controller.gamePad.leftStickAvgY * 0.5f;
				f32 strifeSpeed = controller.gamePad.leftStickAvgX * 0.5f;

				gameState->player.pos = movePlayer(gameState->player, moveSpeed, strifeSpeed);

				f32 rotateSpeed = controller.gamePad.rightStickAvgX * 0.25f;
				gameState->player.dir = rotate2D(gameState->player.dir, rotateSpeed);
				gameState->projectionPlane = rotate2D(gameState->projectionPlane, rotateSpeed);
			}
			else
			{
				f32 rotateSpeed = (f32)controller.mouse.deltaX * 0.025f;
				gameState->player.dir = rotate2D(gameState->player.dir, rotateSpeed);
				gameState->projectionPlane = rotate2D(gameState->projectionPlane, rotateSpeed);

				if (controller.mouse.deltaWheel > 30)
				{
					gameState->activeGun = greenGun;
				}
				else if (controller.mouse.deltaWheel < -30)
				{
					gameState->activeGun = rayGun;
				}
			}
			
			// Digital input processing
			if (controller.moveUp.wasDown )
			{
				f32 moveSpeed = 0.5f;
				gameState->player.pos = movePlayer(gameState->player, moveSpeed, 0);
			}
			if (controller.moveDown.wasDown)
			{
				f32 moveSpeed = -0.5f;
				gameState->player.pos = movePlayer(gameState->player, moveSpeed, 0);
			}
			if (controller.moveLeft.wasDown)
			{
				f32 strifeSpeed = -0.5f;
				gameState->player.pos = movePlayer(gameState->player, 0, strifeSpeed);
			}
			if (controller.moveRight.wasDown)
			{
				f32 strifeSpeed = 0.5f;
				gameState->player.pos = movePlayer(gameState->player, 0, strifeSpeed);
			}
			if (controller.actionFire.wasDown && controller.actionFire.halfTransCount)
			{
				++gameState->soundInfos[LaserBullet].count;
			}
			if (controller.action1.wasDown && controller.action1.halfTransCount)
			{
				gameState->soundMasterVolume = clamp(gameState->soundMasterVolume + 0.1f, 0.0f, 1.0f);
			}
			if (controller.action2.wasDown && controller.action2.halfTransCount)
			{
				gameState->soundMasterVolume = clamp(gameState->soundMasterVolume - 0.1f, 0.0f, 1.0f);
			}
		}
		else
		{
			//TODO: LOG
		}
	}

	//TODO: change to movaps, movsb or other SSE
	memset(buffer->memory, 0, buffer->height*buffer->width*buffer->bytesPerPixel);
	
	{
		//TODO: Refactor, understand and optimize all in scope
		f32 posX = gameState->player.pos.x, posY = gameState->player.pos.y;
		f32 dirX = gameState->player.dir.x, dirY = gameState->player.dir.y;
		f32 planeX = gameState->projectionPlane.x, planeY = gameState->projectionPlane.y;

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
				if(worldMap[mapY][mapX] > 0) hit = 1;
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
			switch(worldMap[mapY][mapX])
			{
				// AA'RR'GG'BB
				case 1:  color = (u32)0xFF'FF'00'00;	break; //red
				case 2:  color = (u32)0x00'00'F0'00;	break; //green
				case 3:  color = (u32)0x00'00'00'FF;	break; //blue
				case 4:  color = (u32)0x0F'0F'FF'FF;	break; //white
				default: color = (u32)0xFF'FF'FF'FF;	break; //yellow
			}

			//give x and y sides different brightness
			if(side == 0) {color = color / 2;}

			//draw the pixels of the stripe as a vertical line
			drawRectangle(buffer, {(f32)x, (f32)drawStart}, {(f32)x+1, (f32)drawEnd}, color);
		}
	}

	drawCrossHair(buffer);
	drawGun(buffer, &gameState->gunTextures[gameState->activeGun]);

	// Fill platform audio buffer
	sounds->masterVolume = gameState->soundMasterVolume;
	sounds->buffer = gameState->soundsBuffer;
	sounds->soundsPlayInfos = gameState->soundInfos;
	sounds->maxSoundAssets = gameState->soundsAssetCount;
}