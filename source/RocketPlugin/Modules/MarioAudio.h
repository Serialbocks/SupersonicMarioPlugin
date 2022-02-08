#pragma once

#include "soloud.h"
#include "soloud_wav.h"
#include "Utils.h"

#define SOUND_MASK_MARIO_YAH_WAH_HOO      0x00000001
#define SOUND_MASK_MARIO_HOOHOO           0x00000002
#define SOUND_MASK_MARIO_YAHOO            0x00000004
#define SOUND_MASK_MARIO_UH               0x00000008
#define SOUND_MASK_MARIO_HRMM             0x00000010
#define SOUND_MASK_MARIO_WAH2             0x00000020
#define SOUND_MASK_MARIO_WHOA             0x00000040
#define SOUND_MASK_MARIO_EEUH             0x00000080
#define SOUND_MASK_MARIO_ATTACKED         0x00000100
#define SOUND_MASK_MARIO_OOOF             0x00000200
#define SOUND_MASK_MARIO_OOOF2            0x00000400
#define SOUND_MASK_MARIO_HERE_WE_GO       0x00000800
#define SOUND_MASK_MARIO_YAWNING          0x00001000
#define SOUND_MASK_MARIO_SNORING1         0x00002000
#define SOUND_MASK_MARIO_SNORING2         0x00004000
#define SOUND_MASK_MARIO_WAAAOOOW         0x00008000
#define SOUND_MASK_MARIO_HAHA             0x00010000
#define SOUND_MASK_MARIO_HAHA_2           0x00020000
#define SOUND_MASK_MARIO_UH2              0x00040000
#define SOUND_MASK_MARIO_UH2_2            0x00080000
#define SOUND_MASK_MARIO_ON_FIRE          0x00100000
#define SOUND_MASK_MARIO_DYING            0x00200000
#define SOUND_MASK_MARIO_PANTING_COLD     0x00400000

typedef struct MarioSound_t
{
	uint32_t mask;
	std::string wavPath;
	SoLoud::Wav wav;
	bool playing = false;
} MarioSound;

class MarioAudio
{
public:
	MarioAudio();
	~MarioAudio();
	void UpdateSounds(int soundMask);

private:
	SoLoud::Wav testWav;
	std::vector<MarioSound> marioSounds = {
		{ SOUND_MASK_MARIO_YAH_WAH_HOO, "sfx_mario/00.wav" },
		{ SOUND_MASK_MARIO_YAHOO, "sfx_mario/04.wav"}
	};
	Utils utils;
};

