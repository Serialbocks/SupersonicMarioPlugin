#include "pch.h"
#include "MarioAudio.h"

MarioAudio::MarioAudio()
{
	soloud = new SoLoud::Soloud();
	soloud->init();

	std::string soundDir = utils.GetBakkesmodFolderPath() + "data\\assets\\sound\\";
	for (auto i = 0; i < soundPaths.size(); i++)
	{
		std::string soundPath = soundDir + soundPaths[i];
		SoLoud::Wav *sound = new SoLoud::Wav();
		sound->load(soundPath.c_str());
		audioSources.push_back(sound);
	}
}

MarioAudio::~MarioAudio()
{
	for (auto i = 0; i < audioSources.size(); i++)
	{
		delete audioSources[i];
	}
	soloud->deinit();
	delete soloud;
}

void MarioAudio::PlaySound(int soundIndex)
{
	if (soundIndex >= 0 && soundIndex < audioSources.size())
	{
		soloud->play(*(audioSources[soundIndex]));
	}
}
