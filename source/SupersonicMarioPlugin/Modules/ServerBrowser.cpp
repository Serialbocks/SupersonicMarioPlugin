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

void getMatchesThread(ServerBrowser* self)
{
	auto response = httpClient->Get(GET_MATCHES_REQUEST);

	std::vector<ServerBrowser::Match*> matchesCopy;
	self->sema.acquire();
	if (response.error() != httplib::Error::Success)
	{
		self->errorLoadingMatches = true;
	}
	else
	{
		self->errorLoadingMatches = false;
		try
		{
			simdjson::ondemand::parser parser;
			auto paddedData = simdjson::padded_string(response.value().body);
			auto doc = parser.iterate(paddedData);
			auto matchesArr = doc.get_array();

			for (simdjson::ondemand::object matchResult : matchesArr)
			{
				ServerBrowser::Match* match = new ServerBrowser::Match();

				auto nameResult = matchResult.find_field("name").get_string();
				if (nameResult.error() != simdjson::error_code::SUCCESS)
					continue;
				match->name = nameResult.value();

				auto playerCountResult = matchResult.find_field("playerCount").get_int64();
				if (playerCountResult.error() != simdjson::error_code::SUCCESS)
					continue;
				match->playerCount = playerCountResult.value();

				auto capacityResult = matchResult.find_field("capacity").get_int64();
				if (capacityResult.error() != simdjson::error_code::SUCCESS)
					continue;
				match->capacity = capacityResult.value();

				auto ipAddressResult = matchResult.find_field("ipAddress").get_string();
				if (ipAddressResult.error() != simdjson::error_code::SUCCESS)
					continue;
				match->ipAddress = ipAddressResult.value();

				auto portResult = matchResult.find_field("port").get_int64();
				if (portResult.error() != simdjson::error_code::SUCCESS)
					continue;
				match->port = portResult.value();

				auto sm64PortResult = matchResult.find_field("sm64Port").get_int64();
				if (sm64PortResult.error() != simdjson::error_code::SUCCESS)
					continue;
				match->sm64Port = sm64PortResult.value();

				matchesCopy.push_back(match);
			}
		}
		catch(std::exception)
		{
			self->errorLoadingMatches = true;
		}
	}

	matchesCopy.swap(self->matches);
	self->loadingMatches = false;
	self->sema.release();
}

void ServerBrowser::GetMatches()
{
	sema.acquire();
	loadingMatches = true;
	
	sema.release();
	std::thread matchesThread(getMatchesThread, this);
	matchesThread.detach();
}

std::vector<const char*> ServerBrowser::GetMatchNames()
{
	sema.acquire();
	std::vector<const char*> matchNames;
	for (auto i = 0; i < matches.size(); i++)
	{
		matchNames.push_back(matches[i]->name.c_str());
	}
	sema.release();
	return matchNames;
}

ServerBrowser::Match* ServerBrowser::GetMatchInfo(int matchIndex)
{
	sema.acquire();
	auto match = matches[matchIndex];
	sema.release();
	return match;
}

bool ServerBrowser::IsLoadingMatches()
{
	bool isLoading = false;
	sema.acquire();
	isLoading = loadingMatches;
	sema.release();
	return isLoading;
}

bool ServerBrowser::HasErrorLoadingMatches()
{
	bool hasError = false;
	sema.acquire();
	hasError = errorLoadingMatches;
	sema.release();
	return hasError;
}

ServerBrowser::ServerBrowser()
{
	
}