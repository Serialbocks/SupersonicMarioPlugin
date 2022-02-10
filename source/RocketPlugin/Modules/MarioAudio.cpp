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

std::pair<size_t, size_t> MarioAudio::Resample(double  factor,
	float* inBuffer,
	size_t  inBufferLen,
	bool    lastFlag,
	float* outBuffer,
	size_t  outBufferLen)
{
	size_t idone, odone;
	soxr_quality_spec_t q_spec;

	q_spec = soxr_quality_spec(SOXR_HQ, SOXR_VR);
	soxrHandle.reset(soxr_create(1, factor, 1, 0, 0, &q_spec, 0));
	soxr_process(soxrHandle.get(),
		inBuffer, (lastFlag ? ~inBufferLen : inBufferLen), &idone,
		outBuffer, outBufferLen, &odone);
	return { idone, odone };
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
