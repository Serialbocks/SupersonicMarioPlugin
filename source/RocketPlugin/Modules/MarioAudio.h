#pragma once

#include "soloud.h"
#include "soloud_wav.h"
#include "Utils.h"

#define SOUND_MARIO_HOO			     0
#define SOUND_MARIO_WAH			     1
#define SOUND_MARIO_YAH			     2
#define SOUND_MARIO_HOOHOO           3
#define SOUND_MARIO_YAHOO            4
#define SOUND_MARIO_UH               5
#define SOUND_MARIO_HRMM             6
#define SOUND_MARIO_WAH2             7
#define SOUND_MARIO_WHOA             8
#define SOUND_MARIO_EEUH             9
#define SOUND_MARIO_ATTACKED         10
#define SOUND_MARIO_OOOF             11
#define SOUND_MARIO_OOOF2            12
#define SOUND_MARIO_HERE_WE_GO       13
#define SOUND_MARIO_YAWNING          14
#define SOUND_MARIO_SNORING1         15
#define SOUND_MARIO_SNORING2         16
#define SOUND_MARIO_WAAAOOOW         17
#define SOUND_MARIO_HAHA             18
#define SOUND_MARIO_HAHA_2           19
#define SOUND_MARIO_UH2              20
#define SOUND_MARIO_UH2_2            21
#define SOUND_MARIO_ON_FIRE          22
#define SOUND_MARIO_DYING            23
#define SOUND_MARIO_PANTING_COLD     24

static std::vector<std::string> soundPaths = {
	"sfx_mario/00.wav", // SOUND_MARIO_HOO
	"sfx_mario/01.wav", // SOUND_MARIO_WAH
	"sfx_mario/02.wav", // SOUND_MARIO_YAH
	"sfx_mario_peach/01.wav", // SOUND_MARIO_HOOHOO
	"sfx_mario/04.wav", // SOUND_MARIO_YAHOO
};

class MarioAudio
{
public:
	MarioAudio();
	~MarioAudio();
	void PlaySound(int soundIndex);

private:
	SoLoud::Soloud* soloud = nullptr;
	SoLoud::Wav testWav;
	std::vector<SoLoud::Wav*> audioSources;
	Utils utils;
};

