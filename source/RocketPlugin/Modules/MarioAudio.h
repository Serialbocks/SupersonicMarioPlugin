#pragma once

#include "soloud.h"
#include "soloud_wav.h"
#include "Utils.h"

#define SOUND_MARIO_YAH				0x00000001
#define SOUND_MARIO_WAH				0x00000002
#define SOUND_MARIO_HOO				0x00000004
#define SOUND_MARIO_YAHOO           0x00000008
#define SOUND_MARIO_HOOHOO          0x00000010

typedef struct MarioSound_t
{
	uint32_t mask;
	std::string wavPath;
	float playbackSpeed = 1.0f;
	SoLoud::Wav wav;
	bool playing = false;
} MarioSound;

class MarioAudio
{
public:
	MarioAudio();
	~MarioAudio();
	void UpdateSounds(int soundMask,
		float aPosX,
		float aPosY,
		float aPosZ,
		float aVelX = 0.0f,
		float aVelY = 0.0f,
		float aVelZ = 0.0f);

private:
	SoLoud::Wav testWav;
	std::vector<MarioSound> marioSounds = {
		{ SOUND_MARIO_YAH,		"sfx_mario/02.wav",			0.91f}, // 07.wav??
		{ SOUND_MARIO_WAH,		"sfx_mario/01.wav",			0.85f},
		{ SOUND_MARIO_HOO,		"sfx_mario/00.wav",			1.08f},
		{ SOUND_MARIO_YAHOO,	"sfx_mario/04.wav",			1.0f},
		{ SOUND_MARIO_HOOHOO,	"sfx_mario_peach/01.wav",	}
	};
	Utils utils;
};

