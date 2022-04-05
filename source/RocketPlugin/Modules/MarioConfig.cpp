#include "MarioConfig.h"
#include "miniconf.h"

miniconf::Config conf;

MarioConfig::MarioConfig()
{
	conf.option(VOLUME_LOOKUP).shortflag("v").defaultValue(VOLUME_DEFAULT).required(true).description("Volume");
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