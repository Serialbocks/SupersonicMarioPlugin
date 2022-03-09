#pragma once

#include "soloud.h"
#include "soloud_wav.h"
#include "Utils.h"

#include <soxr./src/soxr.h>
#pragma comment(lib, "libsoxr.lib")

#define SOUND_MARIO_YAH				0x00000001
#define SOUND_MARIO_WAH				0x00000002
#define SOUND_MARIO_HOO				0x00000004
#define SOUND_MARIO_YAHOO           0x00000008
#define SOUND_MARIO_HOOHOO          0x00000010
#define SOUND_MARIO_PUNCH_HOO       0x00000020
#define SOUND_ACTION_TERRAIN_STEP   0x00000040

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
	std::pair<size_t, size_t> resample(double factor,
		float* inBuffer,
		size_t inBufferLen,
		bool lastFlag,
		float* outBuffer,
		size_t outBufferLen);

private:
	SoLoud::Wav testWav;
	std::vector<MarioSound> marioSounds = {
		{ SOUND_MARIO_YAH,				"sfx_mario/02.wav",					0.91f}, // 07.wav??
		{ SOUND_MARIO_WAH,				"sfx_mario/01.wav",					0.85f},
		{ SOUND_MARIO_HOO,				"sfx_mario/00.wav",					1.08f},
		{ SOUND_MARIO_YAHOO,			"sfx_mario/04.wav",					1.0f}, // changed file, forgot values
		{ SOUND_MARIO_HOOHOO,			"sfx_mario_peach/01.wav",			1.0f}, // 1.0f then 1.21f
		{ SOUND_MARIO_PUNCH_HOO,		"sfx_mario_peach/09.wav",			1.05f},
		{ SOUND_ACTION_TERRAIN_STEP,	"sfx_terrain/00_step_grass.wav",	0.0f}
	};
	Utils utils;
	soxrHandle soxrHandle;
};

