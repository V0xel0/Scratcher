#pragma once

struct GameState
{
	AllocArena worldStorage;

	AllocArena assetsStorage;
	GameSoundAsset *soundsBuffer;
	GameSoundPlayInfo *soundInfos;
	s32 soundsAssetCount;

	s32 colorOffsetX;
	s32 colorOffsetY;

	s32 playerX;
	s32 playerY;

	f32 gravityJump;
};