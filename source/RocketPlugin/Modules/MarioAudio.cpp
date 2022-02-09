#include "pch.h"
#include "MarioAudio.h"

static SoLoud::Soloud* soloud = nullptr;

void onExit(void)
{
	if (soloud != nullptr)
	{
		// Ensure the process fully ends... not great but here we are
		abort();
		soloud->deinit();
	}
}

MarioAudio::MarioAudio()
{
	atexit(onExit);
	soloud = new SoLoud::Soloud();
	soloud->init();

	std::string soundDir = utils.GetBakkesmodFolderPath() + "data\\assets\\sound\\";
	for (auto i = 0; i < marioSounds.size(); i++)
	{
		std::string soundPath = soundDir + marioSounds[i].wavPath;
		marioSounds[i].wav.load(soundPath.c_str());
	}
}

MarioAudio::~MarioAudio()
{
	soloud->deinit();
	delete soloud;
	soloud = nullptr;
}

static volatile float playbackSpeed = 1.0f;
void MarioAudio::UpdateSounds(int soundMask,
	float aPosX,
	float aPosY,
	float aPosZ,
	float aVelX,
	float aVelY,
	float aVelZ)
{
	if (soundMask == 0)
		return;
	for (auto i = 0; i < marioSounds.size(); i++)
	{
		auto marioSound = &marioSounds[i];
		if (marioSound->mask & soundMask)
		{
			int handle = soloud->play3d(marioSound->wav, aPosX, aPosY, aPosZ, aVelX, aVelY, aVelZ, 0.5f);
			soloud->setRelativePlaySpeed(handle, marioSound->playbackSpeed);
		}
	}
}
