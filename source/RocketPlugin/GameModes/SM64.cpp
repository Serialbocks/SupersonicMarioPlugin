#include "SM64.h"

#include <thread>
#include <chrono>
#include <functional>

// CONSTANTS
#define RL_YAW_RANGE 64692
#define CAR_OFFSET_Z 35.0f
#define DEPTH_FACTOR 20000

SM64::SM64(std::shared_ptr<GameWrapper> gw, std::shared_ptr<CVarManagerWrapper> cm, BakkesMod::Plugin::PluginInfo exports)
{
	using namespace std::placeholders;
	gameWrapper = gw;
	cvarManager = cm;
	Netcode = std::make_shared<NetcodeManager>(cvarManager, gameWrapper, exports,
		std::bind(&SM64::OnMessageReceived, this, _1, _2));

	gameWrapper->HookEvent("Function TAGame.Replay_TA.StartPlaybackAtTimeSeconds", bind(&SM64::StopRenderMario, this, _1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccer_TA.ReplayPlayback.BeginState", bind(&SM64::StopRenderMario, this, _1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_TA.OnCarSpawned", bind(&SM64::OnCarSpawned, this, _1));

	InitSM64();
	gameWrapper->RegisterDrawable(std::bind(&SM64::OnRender, this, _1));

	typeIdx = std::make_unique<std::type_index>(typeid(*this));
}

SM64::~SM64()
{
	gameWrapper->UnregisterDrawables();
	DestroySM64();
}

void SM64::StopRenderMario(std::string eventName)
{
	if (marioId >= 0 && renderLocalMario)
	{
		sm64_mario_delete(marioId);
		marioId = -1;
	}
	renderLocalMario = false;
	renderRemoteMario = false;
}

void SM64::OnCarSpawned(std::string eventName)
{

}

void SM64::OnMessageReceived(const std::string& message, PriWrapper sender)
{
	if (sender.IsNull() || sender.IsLocalPlayerPRI())
	{
		return;
	}

	char messageType = message[0];

	char* dest = nullptr;
	if (messageType == 'M')
	{
		dest = (char*)&marioBodyStateIn.marioState;
	}
	else if (messageType == 'B')
	{
		dest = (char*)&marioBodyStateIn;
	}

	if (dest == nullptr)
	{
		return;
	}

	std::string bytesStr = message.substr(1, message.size() - 1);
	auto bytes = hexToBytes(bytesStr);
	for (unsigned int i = 0; i < bytes.size(); i++)
	{
		dest[i] = bytes[i];
	}
	renderRemoteMario = true;
}

/// <summary>Renders the available options for the game mode.</summary>
void SM64::RenderOptions()
{
	//ImGui::Checkbox("Auto Deplete Boost", &autoDeplete);
	//ImGui::SliderInt("Auto Deplete Boost Rate", &autoDepleteRate, 0, 100, "%d boost per second");
}

bool SM64::IsActive()
{
	return isActive;
}

/// <summary>Gets the game modes name.</summary>
/// <returns>The game modes name</returns>
std::string SM64::GetGameModeName()
{
	return "SM64";
}

/// <summary>Activates the game mode.</summary>
void SM64::Activate(const bool active)
{
	if (active && !isActive) {
		HookEventWithCaller<ServerWrapper>(
			"Function GameEvent_Soccar_TA.Active.Tick",
			[this](const ServerWrapper& caller, void* params, const std::string&) {
				onTick(caller);
			});
	}
	else if (!active && isActive) {
		UnhookEvent("Function GameEvent_Soccar_TA.Active.Tick");
	}

	isActive = active;
}

void SM64::InitSM64()
{
	size_t romSize;
	std::string bakkesmodFolderPath = getBakkesmodFolderPath();
	std::string romPath = bakkesmodFolderPath + "data\\assets\\baserom.us.z64";
	uint8_t* rom = utilsReadFileAlloc(romPath, &romSize);
	if (rom == NULL)
	{
		return;
	}
	
	size_t textureSize = 4 * SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT;
	texture = (uint8_t*)malloc(textureSize);
	marioGeometry.position = (float*)malloc(sizeof(float) * 9 * SM64_GEO_MAX_TRIANGLES);
	marioGeometry.color = (float*)malloc(sizeof(float) * 9 * SM64_GEO_MAX_TRIANGLES);
	marioGeometry.normal = (float*)malloc(sizeof(float) * 9 * SM64_GEO_MAX_TRIANGLES);
	marioGeometry.uv = (float*)malloc(sizeof(float) * 6 * SM64_GEO_MAX_TRIANGLES);
	
	//sm64_global_terminate();
	if (!sm64Initialized)
	{
		sm64_global_init(rom, texture, NULL);
		sm64_static_surfaces_load(surfaces, surfaces_count);
		sm64Initialized = true;
	}
	
	marioId = -1;
	remoteMarioId = -1;
	cameraPos[0] = 0.0f;
	cameraPos[1] = 0.0f;
	cameraPos[2] = 0.0f;
	cameraRot = 0.0f;

	locationInit = false;

	renderer = new Renderer();
}

void SM64::DestroySM64()
{
	if (marioId >= 0)
	{
		sm64_mario_delete(marioId);
	}
	marioId = -2;
	//sm64_global_terminate();
	free(texture);
	free(marioGeometry.position);
	free(marioGeometry.color);
	free(marioGeometry.normal);
	free(marioGeometry.uv);
	delete renderer;
}

void SM64::onTick(ServerWrapper server)
{
	for (CarWrapper car : server.GetCars())
	{
		PriWrapper player = car.GetPRI();
		if (!player.GetbMatchAdmin()) continue;

		carLocation = car.GetLocation();
		auto x = (int16_t)(carLocation.X);
		auto y = (int16_t)(carLocation.Y);
		auto z = (int16_t)(carLocation.Z);
		if (marioId < 0)
		{
			carRotation = car.GetRotation();

			// Unreal swaps coords
			marioId = sm64_mario_create(x, z, y);
			if (marioId < 0) continue;
		}

		auto carRot = car.GetRotation();

		//car.SetHidden2(TRUE);
		//car.SetbHiddenSelf(TRUE);
		auto marioYaw = (int)(-marioState.faceAngle * (RL_YAW_RANGE / 6)) + (RL_YAW_RANGE / 4);
		//car.SetLocation(Vector(marioState.posX, marioState.posZ, marioState.posY + CAR_OFFSET_Z));
		//car.SetVelocity(Vector(marioState.velX, marioState.velZ, marioState.velY + CAR_OFFSET_Z));

		carRot.Yaw = marioYaw;
		carRot.Roll = carRotation.Roll;
		carRot.Pitch = carRotation.Pitch;
		//car.SetRotation(carRot);
		
		auto camera = gameWrapper->GetCamera();
		if (!camera.IsNull())
		{
			cameraLoc = camera.GetLocation();
		}

		auto playerController = car.GetPlayerController();
		playerInputs = playerController.GetVehicleInput();
		marioInputs.buttonA = playerInputs.Jump;
		marioInputs.buttonB = playerInputs.Handbrake;
		marioInputs.buttonZ = playerInputs.Throttle < 0;
		marioInputs.stickX = playerInputs.Steer;
		marioInputs.stickY = playerInputs.Pitch;
		marioInputs.camLookX = marioState.posX - cameraLoc.X;
		marioInputs.camLookZ = marioState.posZ - cameraLoc.Y;
		
		sm64_mario_tick(marioId, &marioInputs, &marioState, &marioGeometry, &marioBodyState, true);

		if (marioGeometry.numTrianglesUsed > 0)
		{
			renderLocalMario = true;

			unsigned char* bodyStateBytes = (unsigned char*)&marioBodyState;
			unsigned char* marioStateBytes = (unsigned char*)&marioBodyState.marioState;
			std::string bodyStateStr = "B";
			bodyStateStr += bytesToHex(bodyStateBytes, sizeof(struct SM64MarioBodyState) - sizeof(struct SM64MarioState));
			std::string marioStateStr = "M";
			marioStateStr += bytesToHex(marioStateBytes, sizeof(struct SM64MarioState));

			if (bodyStateStr.length() < 110)
			{
				Netcode->SendNewMessage(bodyStateStr);
			}
			if (marioStateStr.length() < 110)
			{
				Netcode->SendNewMessage(marioStateStr);
			}
		}
	}
}

void SM64::OnRender(CanvasWrapper canvas)
{
	if (renderer == nullptr) return;
	if (!meshesInitialized)
	{
		if (!renderer->Initialized) return;

		marioMesh = renderer->CreateMesh(SM64_GEO_MAX_TRIANGLES,
			texture,
			4 * SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT,
			SM64_TEXTURE_WIDTH,
			SM64_TEXTURE_HEIGHT);

		if (marioMesh == nullptr) return;

		meshesInitialized = true;
		return;
	}

	auto inGame = gameWrapper->IsInGame() || gameWrapper->IsInReplay();
	if ((!renderLocalMario && !renderRemoteMario) || (!inGame && isActive))
	{
		//marioMesh->Render(projectedVertices, 0);
		return;
	}

	auto camera = gameWrapper->GetCamera();
	if (camera.IsNull()) return;

	RT::Frustum frust{ canvas, camera };

	if (renderRemoteMario && remoteMarioId < 0)
	{
		remoteMarioId = sm64_mario_create((int16_t)marioBodyStateIn.marioState.posX,
			(int16_t)marioBodyStateIn.marioState.posY,
			(int16_t)marioBodyStateIn.marioState.posZ);
		if (remoteMarioId < 0) return;
	}

	Vector marioLocation;
	if (renderRemoteMario)
	{
		sm64_mario_tick(remoteMarioId, &marioInputs, &marioBodyStateIn.marioState, &marioGeometry, &marioBodyStateIn, 0);
		marioLocation.X = marioBodyStateIn.marioState.posX;
		marioLocation.Y = marioBodyStateIn.marioState.posY;
		marioLocation.Z = marioBodyStateIn.marioState.posZ;
	}
	else if (renderLocalMario)
	{
		marioLocation.X = marioBodyState.marioState.posX;
		marioLocation.Y = marioBodyState.marioState.posY;
		marioLocation.Z = marioBodyState.marioState.posZ;
	}

	auto currentCameraLocation = camera.GetLocation();
	for (auto i = 0; i < marioGeometry.numTrianglesUsed * 3; i++)
	{
		auto position = &marioGeometry.position[i * 3];
		auto color = &marioGeometry.color[i * 3];
		auto uv = &marioGeometry.uv[i * 2];

		auto currentVertex = &marioMesh->Vertices[i];
		// Unreal engine swaps x and y coords for 3d model
		currentVertex->pos.x = position[0];
		currentVertex->pos.y = position[1];
		currentVertex->pos.z = position[2];
		currentVertex->color.x = color[0];
		currentVertex->color.y = color[1];
		currentVertex->color.z = color[2];
		currentVertex->color.w = 1.0f;
		currentVertex->texCoord.x = uv[0];
		currentVertex->texCoord.y = uv[1];
		
	}
	
	marioMesh->Render(
		marioGeometry.numTrianglesUsed,
		camera,
		marioLocation
	);

}

float SM64::distance(Vector v1, Vector v2)
{
	return (float)sqrt(pow(v2.X - v1.X, 2.0) + pow(v2.Y - v1.Y, 2.0) + pow(v2.Z - v1.Z, 2.0));
}

std::string SM64::bytesToHex(unsigned char* data, unsigned int len)
{
	std::stringstream ss;
	ss << std::hex << std::setfill('0');
	for (unsigned int i = 0; i < len; ++i)
		ss << std::setw(2) << static_cast<unsigned>(data[i]);

	return ss.str();
}


std::vector<char> SM64::hexToBytes(const std::string& hex) {
	std::vector<char> bytes;

	for (unsigned int i = 0; i < hex.length(); i += 2) {
		std::string byteString = hex.substr(i, 2);
		char byte = (char)strtol(byteString.c_str(), NULL, 16);
		bytes.push_back(byte);
	}

	return bytes;
}

void SM64::parseObjFile(std::string path, std::vector<Mesh::MeshVertex>* meshVertices)
{
	std::ifstream file(path);
	std::string line;

	std::vector<Vector> vertices;
	while (std::getline(file, line))
	{
		if (line.size() == 0) continue;
		auto split = splitStr(line, ' ');
		auto type = split[0];
		auto lastIndex = split.size() - 1;
		if (type == "v")
		{
			Vector vertex;
			vertex.X = std::stof(split[lastIndex - 2], nullptr);
			vertex.Y = std::stof(split[lastIndex - 1], nullptr);
			vertex.Z = std::stof(split[lastIndex], nullptr);
			vertices.push_back(vertex);
		}
		else if (type == "f")
		{
			auto vertIndex1 = std::stoul(splitStr(split[lastIndex - 2], '/')[0], nullptr);
			auto vertIndex2 = std::stoul(splitStr(split[lastIndex - 1], '/')[0], nullptr);
			auto vertIndex3 = std::stoul(splitStr(split[lastIndex], '/')[0], nullptr);
			meshVertices->push_back({ vertices[vertIndex1], 1.0f, 0, 0, 1.0f, 1.0f, 1.0f });
		}
	}
	
}

std::vector<std::string> SM64::splitStr(std::string str, char delimiter)
{
	std::stringstream stringStream(str);
	std::vector<std::string> seglist;
	std::string segment;
	while (std::getline(stringStream, segment, delimiter))
	{
		seglist.push_back(segment);
	}
	return seglist;
}

std::string SM64::getBakkesmodFolderPath()
{
	wchar_t szPath[MAX_PATH];
	wchar_t* s = szPath;
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, (LPWSTR)szPath)))
	{
		PathAppend((LPWSTR)szPath, _T("\\bakkesmod\\bakkesmod\\"));

		std::ostringstream stm;

		while (*s != L'\0') {
			stm << std::use_facet< std::ctype<wchar_t> >(std::locale()).narrow(*s++, '?');
		}
		return stm.str();
	}
	return "";
}

uint8_t* SM64::utilsReadFileAlloc(std::string path, size_t* fileLength)
{
	FILE* f;
	fopen_s(&f, path.c_str(), "rb");

	if (!f) return NULL;

	fseek(f, 0, SEEK_END);
	size_t length = (size_t)ftell(f);
	rewind(f);
	uint8_t* buffer = (uint8_t*)malloc(length + 1);
	if (buffer != NULL)
	{
		fread(buffer, 1, length, f);
		buffer[length] = 0;
	}

	fclose(f);

	if (fileLength) *fileLength = length;

	return (uint8_t*)buffer;
}

