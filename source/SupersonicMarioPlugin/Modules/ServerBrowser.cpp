#include "ServerBrowser.h"


#define HOST_MATCH_REQUEST "/host-match?name=%s&capacity=%d&port=%d&sm64Port=%d"
#define GET_MATCHES_REQUEST "/get-matches"


void hostNewMatchThread(ServerBrowser* self, std::string name, int capacity, int port, int sm64Port)
{
	std::string hostMatchUrl = Utils::StringFormat(HOST_MATCH_REQUEST, name, capacity, port, sm64Port);
	auto response = httpClient->Get(hostMatchUrl.c_str());
	if (response.error() != httplib::Error::Success)
	{
		
	}
	
}

void ServerBrowser::HostNewMatch(std::string name, int capacity, int port, int sm64Port)
{
	std::thread newMatchThread(hostNewMatchThread, this, name, capacity, port, sm64Port);
	newMatchThread.detach();
}

ServerBrowser::ServerBrowser()
{
	httpClient = &https;
}