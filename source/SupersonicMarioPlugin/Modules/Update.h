#pragma once

#include <fstream>
#include <string>
#include <iostream>
#include <semaphore>

#pragma comment(lib, "libcrypto.lib")
#pragma comment(lib, "libssl.lib")

#include "simdjson/singleheader/simdjson.h"
#include "cpp-httplib/httplib.h"

#include "Version.h"
#include "Utils.h"
#include "Networking/Networking.h"

class Update
{
public:
	static Update& getInstance()
	{
		static Update instance;
		return instance;
	}
	void CheckForUpdates();
	bool NeedsUpdate();
	std::string GetCurrentVersion();
	bool CheckingForUpdates();
	void GetUpdate();
	bool Updating();

private:
	Update();

public:

private:


	
};