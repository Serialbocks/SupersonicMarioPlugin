#pragma once

#include "Modules/Utils.h"
#include "Networking/Networking.h"
#include <string>
#include <thread>

class ServerBrowser
{
public:
	static ServerBrowser& getInstance()
	{
		static ServerBrowser instance;
		return instance;
	}
	void HostNewMatch(std::string name, int capacity, int port, int sm64Port);

private:
	ServerBrowser();
};
