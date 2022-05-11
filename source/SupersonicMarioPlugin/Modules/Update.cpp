#include "Update.h"

#define VERSION_REQUEST "/version"

httplib::Client http("http://136.32.164.93:3000");
httplib::Client https("https://serialbocks.com");
httplib::Client* httpClient = nullptr;
std::counting_semaphore<1> sema{ 1 };
std::string updatePath;
std::string currentVersion;
std::string baseUrl;
std::string backupBaseUrl;
bool hasCheckedForUpdates = false;
bool checkingForUpdates = false;
bool needsUpdate = false;
bool updating = false;

const std::string installedVersion = VER_PRODUCT_VERSION_STR;

Update::Update()
{
	httpClient = &https;
}

void checkUpdatesThread(Update* self)
{
	auto response = httpClient->Get(VERSION_REQUEST);
	if (response.error() != httplib::Error::Success)
	{
		// Fall back to http if we need
		httpClient = &http;
		response = httpClient->Get(VERSION_REQUEST);
	}
	if (response.error() != httplib::Error::Success)
	{
		sema.acquire();
		checkingForUpdates = false;
		sema.release();
		return;
	}

	simdjson::ondemand::parser parser;
	const simdjson::padded_string paddedData = response.value().body;
	simdjson::ondemand::document doc = parser.iterate(paddedData);
	currentVersion = std::string(std::string_view(doc["number"]));
	updatePath = std::string(std::string_view(doc["link"]));
	baseUrl = std::string(std::string_view(doc["baseUrl"]));
	backupBaseUrl = std::string(std::string_view(doc["backupBaseUrl"]));

	sema.acquire();
	hasCheckedForUpdates = true;
	checkingForUpdates = false;
	needsUpdate = installedVersion.compare(currentVersion) != 0;
	sema.release();
}

void Update::CheckForUpdates()
{
	sema.acquire();
	auto alreadyChecked = checkingForUpdates;
	sema.release();

	if (alreadyChecked)
		return;

	checkingForUpdates = true;
	std::thread checkUpdate(checkUpdatesThread, this);
	checkUpdate.detach();
}

bool Update::NeedsUpdate()
{
	sema.acquire();
	auto needUpdate = needsUpdate;
	sema.release();
	return needUpdate;
}

bool Update::CheckingForUpdates()
{
	sema.acquire();
	auto checkingForUpdate = checkingForUpdates;
	sema.release();
	return checkingForUpdate;
}

std::string Update::GetCurrentVersion()
{
	sema.acquire();
	std::string curVers = currentVersion;
	sema.release();
	return curVers;
}

void updateThread(Update* self)
{
	auto currentClient = httpClient;
	auto baseHttp = httplib::Client(baseUrl);
	auto backupBaseHttp = httplib::Client(backupBaseUrl);

	bool hasBaseUrl = baseUrl.size() > 0;

	if (hasBaseUrl) {
		currentClient = &baseHttp;
	}

	auto response = currentClient->Get(updatePath.c_str());

	if (response.error() != httplib::Error::Success)
	{
		currentClient = &backupBaseHttp;
		response = currentClient->Get(updatePath.c_str());
	}

	if (response.error() == httplib::Error::Success)
	{
		auto updatePath = Utils::GetBakkesmodFolderPath() + "data\\assets\\supersonic-mario-update.exe";

		std::string body = response.value().body;
		std::ofstream out(updatePath, std::ios::out | std::ios::binary);
		out.write(body.c_str(), sizeof(char) * body.size());
		out.close();

		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));
		CreateProcessA(NULL, (LPSTR)updatePath.c_str(), NULL, NULL,
			FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
	}

	sema.acquire();
	updating = false;
	sema.release();
}

void Update::GetUpdate()
{
	if (Updating())
		return;

	updating = true;
	std::thread getUpdate(updateThread, this);
	getUpdate.detach();
}

bool Update::Updating()
{
	sema.acquire();
	bool isUpdating = updating;
	sema.release();
	return isUpdating;
}