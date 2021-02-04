#pragma once
struct World
{
	s32 width;
	s32 height;
	byte *data;
};

struct GameState
{
	AllocArena worldStorage;
	World* world;

	AllocArena assetsStorage;
	GameSoundAsset *soundsBuffer;
	GameSoundPlayInfo *soundInfos;
	s32 soundsAssetCount;

	s32 colorOffsetX;
	s32 colorOffsetY;

	Vec2 playerPosition;
	Vec2 playerDirection;
	Vec2 projectionPlane;

	f32 gravityJump;
};