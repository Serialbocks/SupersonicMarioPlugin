#pragma once

#include "Modules/Utils.h"
#include "Networking/Networking.h"
#include <string>
#include <thread>
#include <vector>

#define MAX_CONCURRENT_MATCHES 500

class ServerBrowser
{
public:
	struct Match
	{
		std::string name;
		int playerCount;
		int capacity;
		std::string ipAddress;
		int port;
		int sm64Port;
	};

public:
	static ServerBrowser& getInstance()
	{
		static ServerBrowser instance;
		return instance;
	}
	void HostNewMatch(std::string name, int capacity, int port, int sm64Port);
	void GetMatches();
	std::vector<const char*> GetMatchNames();
	bool IsLoadingMatches();
	bool HasErrorLoadingMatches();
	Match* GetMatchInfo(int matchIndex);

private:
	ServerBrowser();

public:
	std::counting_semaphore<1> sema{ 1 };
	std::vector<Match*> matches;
	bool loadingMatches = false;
	bool errorLoadingMatches = false;

};
