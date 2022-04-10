#include "pch.h"
#include "MarioAudio.h"

#define ATTEN_ROLLOFF_FACTOR_EXP 0.0003f
#define ATTEN_ROLLOFF_FACTOR_LIN 0
#define PLAYBACK_SPEED_FACTOR 0.01f

static SoLoud::Soloud* soloud = nullptr;

MarioAudio::MarioAudio()
{
	if (soloud == nullptr)
	{
		soloud = new SoLoud::Soloud();
		soloud->init(SoLoud::Soloud::FLAGS::LEFT_HANDED_3D);

		loadSoundFiles();
		soloud->set3dListenerUp(0, 0, 1.0f);
		MasterVolume = MarioConfig::getInstance().GetVolume();
	}
}

MarioAudio::~MarioAudio()
{

}

void MarioAudio::UpdateSounds(int soundMask,
	Vector sourcePos,
	Vector sourceVel,
	Vector listenerPos,
	Vector listenerAt,
	int *inSlideHandle,
	int *yahooHandle,
	uint32_t marioAction)
{
	if (marioAction == ACT_WALL_KICK_AIR)
	{
		marioSounds[SOUND_MARIO_UH_INDEX].wav.stop();
		marioSounds[SOUND_MARIO_DOH_INDEX].wav.stop();
	}

	int slideHandle = -1;
	for (auto i = 0; i < marioSounds.size(); i++)
	{
		auto marioSound = &marioSounds[i];
		if (marioSound->mask & soundMask)
		{
			float distance = utils.Distance(sourcePos, listenerPos);
			float volume = marioSound->volume - (distance * ATTEN_ROLLOFF_FACTOR_LIN) - pow(distance * ATTEN_ROLLOFF_FACTOR_EXP, 2);
			volume = volume <= 0.0f ? 0.0f : volume;
			volume *= MasterVolume / 100.0f;

			if (marioSound->mask == SOUND_MOVING_TERRAIN_SLIDE)
			{
				// Handle sliding as a special case since it loops
				if (*inSlideHandle < 0)
				{
					slideHandle = soloud->play3d(marioSound->wav, sourcePos.X, sourcePos.Y, sourcePos.Z, 0, 0, 0, volume);
				}
				else
				{
					slideHandle = *inSlideHandle;
				}

				float speed = sqrt(sourceVel.X * sourceVel.X +
					sourceVel.Y * sourceVel.Y +
					sourceVel.Z * sourceVel.Z);
				auto playbackSpeed = marioSound->playbackSpeed;
				playbackSpeed += (speed * PLAYBACK_SPEED_FACTOR);
				soloud->setRelativePlaySpeed(slideHandle, marioSound->playbackSpeed);

			}
			else
			{
				int handle = soloud->play3d(marioSound->wav, sourcePos.X, sourcePos.Y, sourcePos.Z, 0, 0, 0, volume);

				auto playbackSpeed = marioSound->playbackSpeed;
				if (marioSound->playbackSpeed == 0.0f)
				{
					// Pick a random number
					float randPercentage = (rand() % 101) / 100.0f;
					float speedChange = 0.07f * randPercentage;
					playbackSpeed = 1.05f + speedChange;
				}

				if (marioSound->mask == SOUND_MARIO_YAHOO)
				{
					if (*yahooHandle >= 0)
					{
						soloud->stop(*yahooHandle);
					}
					*yahooHandle = handle;
				}

				
				soloud->setRelativePlaySpeed(handle, playbackSpeed);
			}

		}
	}

	if (*inSlideHandle >= 0 && slideHandle < 0)
	{
		soloud->stop(*inSlideHandle);
	}

	soloud->set3dListenerPosition(listenerPos.X, listenerPos.Y, listenerPos.Z);
	soloud->set3dListenerAt(listenerAt.X, listenerAt.Y, listenerAt.Z);
	soloud->update3dAudio();

	*inSlideHandle = slideHandle;
}

void MarioAudio::loadSoundFiles()
{
	std::string soundDir = SOUND_DIR;
	for (auto i = 0; i < marioSounds.size(); i++)
	{
		std::string soundPath = soundDir + marioSounds[i].wavPath;
		marioSounds[i].wav.load(soundPath.c_str());
	}

	// Resample certain sounds where altering playback speed isn't good enough to make it sound like the original
	doubleResample(&marioSounds[SOUND_MARIO_YAHOO_INDEX].wav, 4160, 1.04f, 10992, 0.96f);
	doubleResample(&marioSounds[SOUND_MARIO_HOOHOO_INDEX].wav, 2560, 1.0f, 7392, 0.83f);

	// Handle mario landing doubling of step sound
	auto stepGrass = &marioSounds[SOUND_ACTION_TERRAIN_LANDING_INDEX].wav;
	size_t doubleStepGrassSize = (size_t)stepGrass->mSampleCount * 2 * sizeof(float);
	float* doubleStepGrass = (float*)malloc(doubleStepGrassSize);

	if (doubleStepGrass != nullptr)
	{
		ZeroMemory(doubleStepGrass, doubleStepGrassSize);
		memcpy(doubleStepGrass, stepGrass->mData, stepGrass->mSampleCount * sizeof(float));
		memcpy(&(doubleStepGrass[stepGrass->mSampleCount]), stepGrass->mData, stepGrass->mSampleCount * sizeof(float));

		stepGrass->mData = doubleStepGrass;
		stepGrass->mSampleCount *= 2;

		doubleResample(stepGrass, 1216, 0.82f, 1216, 1.03f);

		marioSounds[SOUND_ACTION_TERRAIN_BODY_HIT_GROUND_INDEX].wav.mData = stepGrass->mData;
		marioSounds[SOUND_ACTION_TERRAIN_BODY_HIT_GROUND_INDEX].wav.mSampleCount = stepGrass->mSampleCount;
	}
}

void MarioAudio::doubleResample(SoLoud::Wav* targetSoundWav,
	size_t firstResampleSourceCount,
	float firstResampleFactor,
	size_t secondResampleSourceCount,
	float secondResampleFactor)
{
	size_t firstResampleDestCount = (size_t)firstResampleSourceCount * firstResampleFactor;
	float* firstResampleSource = targetSoundWav->mData;

	size_t secondResampleDestCount = (size_t)secondResampleSourceCount * secondResampleFactor;
	float* secondResampleSource = &(firstResampleSource[firstResampleSourceCount]);

	float* resampleDest = (float*)malloc((firstResampleDestCount + secondResampleDestCount) * sizeof(float));
	auto firstResult = resample(firstResampleFactor, firstResampleSource, firstResampleSourceCount, true, resampleDest, firstResampleDestCount);

	if (resampleDest != nullptr)
	{
		float* secondResampleDest = &(resampleDest[firstResult.second]);
		auto secondResult = resample(secondResampleFactor, secondResampleSource, secondResampleSourceCount, true, secondResampleDest, secondResampleDestCount);

		targetSoundWav->mData = resampleDest;
		targetSoundWav->mSampleCount = firstResult.second + secondResult.second;
	}
}

std::pair<size_t, size_t> MarioAudio::resample(double factor,
	float* inBuffer,
	size_t inBufferLen,
	bool lastFlag,
	float* outBuffer,
	size_t outBufferLen)
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