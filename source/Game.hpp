#pragma once

global_variable constexpr s32 MAX_TEXTURES = 3;

enum GunType
{
	rayGun,
	greenGun,
	CountOfGunTypes
};

struct World
{
	s32 width;
	s32 height;
	byte *data;
};

struct Texture
{
	s32 width;
	s32 height;
	s32 size;
	u32 *pixels;
};

struct GunTexture
{
	s32 alignX;
	s32 alignY;
	Texture anim0;
};

struct Player
{
	Vec2 pos;
	Vec2 dir;
};
struct GameState
{
	AllocArena worldStorage;
	World* world;

	AllocArena assetsStorage;
	GameSoundAsset *soundsBuffer;
	GameSoundPlayInfo *soundInfos;
	f32 soundMasterVolume;
	s32 soundsAssetCount;

	GunTexture gunTextures[MAX_TEXTURES];
	GunType activeGun;

	Player player;
	Vec2 projectionPlane;
};