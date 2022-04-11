#pragma once

#include "Utils.h"

#define CONFIG_FILE_NAME "data\\supersonicmario.cfg"

// Config file keys
#define VOLUME_LOOKUP "volume"
#define VOLUME_DEFAULT 50
#define ROM_LOOKUP "rom"

class MarioConfig
{
public:
	static MarioConfig& getInstance()
	{
		static MarioConfig instance;
		return instance;
	}
	void SetVolume(int volume);
	int GetVolume();
	void SetRomPath(std::string romPath);
	std::string GetRomPath();

private:
	MarioConfig();
	std::string configPath;
	
};