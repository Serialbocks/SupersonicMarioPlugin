#include "MarioConfig.h"
#include "miniconf.h"

miniconf::Config conf;
Utils utils;

static std::string defaultRomPath = utils.GetBakkesmodFolderPath() + "data\\assets\\baserom.us.z64";

MarioConfig::MarioConfig()
{
	conf.option(VOLUME_LOOKUP).shortflag("v").defaultValue(VOLUME_DEFAULT).required(true).description("Volume");
	conf.option(ROM_LOOKUP).shortflag("r").defaultValue(defaultRomPath).required(true).description("Rom");
	configPath = utils.GetBakkesmodFolderPath() + CONFIG_FILE_NAME;
	conf.config(configPath);
}

void MarioConfig::SetVolume(int volume)
{
	conf[VOLUME_LOOKUP] = volume;
	conf.serialize(configPath);
}

int MarioConfig::GetVolume()
{
	auto volConf = conf[VOLUME_LOOKUP];
	if (volConf.isEmpty())
	{
		SetVolume(VOLUME_DEFAULT);
	}
	return conf[VOLUME_LOOKUP].getInt();
}

void MarioConfig::SetRomPath(std::string romPath)
{
	std::string newPath = romPath;
	conf[ROM_LOOKUP] = romPath;
	conf.serialize(configPath);
}

std::string MarioConfig::GetRomPath()
{
	auto romConf = conf[ROM_LOOKUP];
	if (romConf.isEmpty())
	{
		SetRomPath(defaultRomPath);
	}
	std::string result = conf[ROM_LOOKUP].getString();
	Utils::trim(result);
	return result;
}