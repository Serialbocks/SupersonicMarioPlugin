#pragma once

#include "miniconf.h"
#include "Utils.h"

#define CONFIG_FILE_NAME "data\\sm64_config.csv"

class MarioConfig
{
public:
	MarioConfig();

private:
	Utils utils;
};