#pragma once

#include "soloud.h"
#include "soloud_wav.h"
#include "Utils.h"

#include <soxr./src/soxr.h>
#pragma comment(lib, "libsoxr.lib")

#define SOUND_MARIO_YAH							0x00000001
#define SOUND_MARIO_WAH							0x00000002
#define SOUND_MARIO_HOO							0x00000004
#define SOUND_MARIO_YAHOO						0x00000008
#define SOUND_MARIO_HOOHOO						0x00000010
#define SOUND_MARIO_PUNCH_YAH					0x00000020
#define SOUND_MARIO_PUNCH_WAH					0x00000040
#define SOUND_MARIO_PUNCH_HOO					0x00000080
#define SOUND_ACTION_TERRAIN_STEP				0x00000100
#define SOUND_ACTION_SPIN						0x00000200
#define SOUND_ACTION_TERRAIN_HEAVY_LANDING      0x00000400
#define SOUND_MARIO_GROUND_POUND_WAH            0x00000800
#define SOUND_ACTION_SIDE_FLIP_UNK              0x00001000
#define SOUND_MARIO_HAHA                        0x00002000
#define SOUND_ACTION_TERRAIN_LANDING            0x00004000
#define SOUND_MOVING_TERRAIN_SLIDE              0x00008000
#define SOUND_ACTION_BOUNCE_OFF_OBJECT          0x00010000

struct soxr;
extern "C" void soxr_delete(soxr*);
struct soxr_deleter {
	void operator () (soxr* p) const { if (p) soxr_delete(p); }
};
using soxrHandle = std::unique_ptr<soxr, soxr_deleter>;

typedef struct MarioSound_t
{
	uint32_t mask;
	std::string wavPath;
	float playbackSpeed = 1.0f;
	float loopbackPosMs = -1.0f;
	SoLoud::Wav wav;
	bool playing = false;
} MarioSound;

class MarioAudio
{
public:
	MarioAudio();
	~MarioAudio();
	int UpdateSounds(int soundMask,
		Vector sourcePos,
		Vector listenerPos,
		Vector listenerAt,
		int inSlidingHandle,
		float aVelX = 0.0f,
		float aVelY = 0.0f,
		float aVelZ = 0.0f);
private:
	std::pair<size_t, size_t> resample(double factor,
		float* inBuffer,
		size_t inBufferLen,
		bool lastFlag,
		float* outBuffer,
		size_t outBufferLen);

private:
	SoLoud::Wav testWav;
	std::vector<MarioSound> marioSounds = {
		{ SOUND_MARIO_YAH,							"sfx_mario/02.wav",					0.91f},
		{ SOUND_MARIO_WAH,							"sfx_mario/01.wav",					0.85f},
		{ SOUND_MARIO_HOO,							"sfx_mario/00.wav",					1.08f},
		{ SOUND_MARIO_YAHOO,						"sfx_mario/04.wav",					1.0f}, // changed file, forgot values
		{ SOUND_MARIO_HOOHOO,						"sfx_mario_peach/01.wav",			1.0f}, // 1.0f then 1.21f
		{ SOUND_MARIO_PUNCH_YAH,					"sfx_mario/02.wav",					0.91f},
		{ SOUND_MARIO_PUNCH_WAH,					"sfx_mario/01.wav",					0.85f},
		{ SOUND_MARIO_PUNCH_HOO,					"sfx_mario_peach/09.wav",			1.05f},
		{ SOUND_ACTION_TERRAIN_STEP,				"sfx_terrain/01_step_grass.wav",	0.0f},
		{ SOUND_ACTION_SPIN,						"sfx_1/00_twirl.wav",				1.14f},
		{ SOUND_ACTION_TERRAIN_HEAVY_LANDING,		"sfx_1/05_heavy_landing.wav",		1.13f},
		{ SOUND_MARIO_GROUND_POUND_WAH,				"sfx_mario/07.wav",					0.91f},
		{ SOUND_ACTION_SIDE_FLIP_UNK,				"sfx_1/00_twirl.wav",				1.12f},
		{ SOUND_MARIO_HAHA,							"sfx_mario/03.wav",					1.0f},
		{ SOUND_ACTION_TERRAIN_LANDING,				"sfx_terrain/01_step_grass.wav",	1.05f},
		{ SOUND_MOVING_TERRAIN_SLIDE,				"sfx_4/01.wav",						1.1f,			0.051f},
		// TODO: SOUND_MOVING_TERRAIN_SLIDE
	};
	Utils utils;
	soxrHandle soxrHandle;
};

