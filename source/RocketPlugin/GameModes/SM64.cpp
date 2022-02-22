#include "SM64.h"

#include <thread>
#include <chrono>
#include <functional>

// CONSTANTS
#define RL_YAW_RANGE 64692
#define CAR_OFFSET_Z 45.0f
#define BALL_MODEL_SCALE 5.35f
#define SM64_TEXTURE_SIZE (4 * SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT)

inline void tickMarioInstance(SM64MarioInstance* marioInstance, CarWrapper car, SM64* instance);

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
	if (localMario.marioId >= 0 && renderLocalMario)
	{
		sm64_mario_delete(localMario.marioId);
		localMario.marioId = -1;
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

	auto playerName = sender.GetPlayerName().ToString();
	auto marioIterator = remoteMarios.find(playerName);
	SM64MarioInstance* marioInstance = nullptr;
	if (remoteMarios.count(playerName) == 0)
	{
		// Initialize mario for this player
		marioInstance = new SM64MarioInstance();
		remoteMarios[playerName] = marioInstance;
	}
	else
	{
		marioInstance = remoteMarios[playerName];
	}

	if (marioInstance == nullptr) return;

	char messageType = message[0];

	char* dest = nullptr;
	if (messageType == 'M')
	{
		dest = (char*)&marioInstance->marioBodyState.marioState;
	}
	else if (messageType == 'B')
	{
		dest = (char*)&marioInstance->marioBodyState;
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

#define MIN_LIGHT_COORD -6000.0f
#define MAX_LIGHT_COORD 6000.0f
static int currentLightIndex = 0;
/// <summary>Renders the available options for the game mode.</summary>
void SM64::RenderOptions()
{
	if (renderer != nullptr)
	{
		ImGui::Text("Ambient Light");
		ImGui::SliderFloat("R", &renderer->Lighting.AmbientLightColorR, 0.0f, 1.0f);
		ImGui::SliderFloat("G", &renderer->Lighting.AmbientLightColorG, 0.0f, 1.0f);
		ImGui::SliderFloat("B", &renderer->Lighting.AmbientLightColorB, 0.0f, 1.0f);
		ImGui::SliderFloat("Ambient Strength", &renderer->Lighting.AmbientLightStrength, 0.0f, 1.0f);

		ImGui::NewLine();

		ImGui::Text("Dynamic Light");
		std::string currentLightLabel = "Light " + std::to_string(currentLightIndex + 1);
		if (ImGui::BeginCombo("Light Select", currentLightLabel.c_str()))
		{
			for (int i = 0; i < MAX_LIGHTS; i++)
			{
				std::string lightLabel = "Light " + std::to_string(i + 1);
				bool isSelected = i == currentLightIndex;
				if (ImGui::Selectable(lightLabel.c_str(), isSelected))
					currentLightIndex = i;
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::SliderFloat("X", &renderer->Lighting.Lights[currentLightIndex].posX, MIN_LIGHT_COORD, MAX_LIGHT_COORD);
		ImGui::SliderFloat("Y", &renderer->Lighting.Lights[currentLightIndex].posY, MIN_LIGHT_COORD, MAX_LIGHT_COORD);
		ImGui::SliderFloat("Z", &renderer->Lighting.Lights[currentLightIndex].posZ, MIN_LIGHT_COORD, MAX_LIGHT_COORD);
		ImGui::SliderFloat("Rd", &renderer->Lighting.Lights[currentLightIndex].r, 0.0f, 1.0f);
		ImGui::SliderFloat("Gd", &renderer->Lighting.Lights[currentLightIndex].g, 0.0f, 1.0f);
		ImGui::SliderFloat("Bd", &renderer->Lighting.Lights[currentLightIndex].b, 0.0f, 1.0f);
		ImGui::SliderFloat("Dynamic Strength", &renderer->Lighting.Lights[currentLightIndex].strength, 0.0f, 1.0f);
		ImGui::Checkbox("Show Bulb", &renderer->Lighting.Lights[currentLightIndex].showBulb);
	}
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
	std::string bakkesmodFolderPath = utils.GetBakkesmodFolderPath();
	std::string romPath = bakkesmodFolderPath + "data\\assets\\baserom.us.z64";
	uint8_t* rom = utilsReadFileAlloc(romPath, &romSize);
	if (rom == NULL)
	{
		return;
	}
	
	texture = (uint8_t*)malloc(SM64_TEXTURE_SIZE);
	
	//sm64_global_terminate();
	if (!sm64Initialized)
	{
		sm64_global_init(rom, texture, NULL);
		sm64_static_surfaces_load(surfaces, surfaces_count);
		sm64Initialized = true;
	}
	
	cameraPos[0] = 0.0f;
	cameraPos[1] = 0.0f;
	cameraPos[2] = 0.0f;
	cameraRot = 0.0f;

	locationInit = false;

	renderer = new Renderer();
	marioAudio = new MarioAudio();
}

void SM64::DestroySM64()
{
	if (localMario.marioId >= 0)
	{
		sm64_mario_delete(localMario.marioId);
	}
	localMario.marioId = -2;
	//sm64_global_terminate();
	free(texture);
	delete renderer;
	delete marioAudio;
}

void SM64::onTick(ServerWrapper server)
{
	for (CarWrapper car : server.GetCars())
	{
		PriWrapper player = car.GetPRI();
		if (player.IsNull()) continue;
		if (player.GetbMatchAdmin())
		{
			tickMarioInstance(&localMario, car, this);
			renderLocalMario = true;
		}
		else
		{
			auto playerNamePtr = player.GetPlayerName();
			if (playerNamePtr.IsNull()) continue;
			auto playerName = playerNamePtr.ToString();

			if (remoteMarios.count(playerName) > 0)
			{
				SM64MarioInstance* marioInstance = remoteMarios[playerName];
				car.SetHidden2(TRUE);
				car.SetbHiddenSelf(TRUE);

				auto marioYaw = (int)(-marioInstance->marioState.faceAngle * (RL_YAW_RANGE / 6)) + (RL_YAW_RANGE / 4);
				auto carPosition = Vector(marioInstance->marioState.posX, marioInstance->marioState.posZ, marioInstance->marioState.posY + CAR_OFFSET_Z);
				car.SetLocation(carPosition);
				car.SetVelocity(Vector(marioInstance->marioState.velX, marioInstance->marioState.velZ, marioInstance->marioState.velY));
			}
		}

		
	}
}

inline void tickMarioInstance(SM64MarioInstance* marioInstance, CarWrapper car, SM64* instance)
{
	if (car.IsNull()) return;
	auto carLocation = car.GetLocation();
	auto x = (int16_t)(carLocation.X);
	auto y = (int16_t)(carLocation.Y);
	auto z = (int16_t)(carLocation.Z);
	auto carRotation = car.GetRotation();
	if (marioInstance->marioId < 0)
	{
		// Unreal swaps coords
		marioInstance->marioId = sm64_mario_create(x, z, y);
		if (marioInstance->marioId < 0) return;
	}

	auto airControl = car.GetAirControlComponent();
	if (!airControl.IsNull())
	{
		airControl.SetDodgeDisableTimeRemaining(0.0f);
	}

	car.SetHidden2(TRUE);
	car.SetbHiddenSelf(TRUE);

	auto marioYaw = (int)(-marioInstance->marioState.faceAngle * (RL_YAW_RANGE / 6)) + (RL_YAW_RANGE / 4);
	auto carPosition = Vector(marioInstance->marioState.posX, marioInstance->marioState.posZ, marioInstance->marioState.posY + CAR_OFFSET_Z);
	car.SetLocation(carPosition);
	car.SetVelocity(Vector(marioInstance->marioState.velX, marioInstance->marioState.velZ, marioInstance->marioState.velY));

	auto carRot = car.GetRotation();
	carRot.Yaw = marioYaw;
	carRot.Roll = carRotation.Roll;
	carRot.Pitch = carRotation.Pitch;
	car.SetRotation(carRot);

	auto camera = instance->gameWrapper->GetCamera();
	if (!camera.IsNull())
	{
		instance->cameraLoc = camera.GetLocation();
	}

	auto playerController = car.GetPlayerController();
	instance->playerInputs = playerController.GetVehicleInput();
	marioInstance->marioInputs.buttonA = instance->playerInputs.Jump;
	marioInstance->marioInputs.buttonB = instance->playerInputs.Handbrake;
	marioInstance->marioInputs.buttonZ = instance->playerInputs.Throttle < 0;
	marioInstance->marioInputs.stickX = instance->playerInputs.Steer;
	marioInstance->marioInputs.stickY = instance->playerInputs.Pitch;
	marioInstance->marioInputs.camLookX = marioInstance->marioState.posX - instance->cameraLoc.X;
	marioInstance->marioInputs.camLookZ = marioInstance->marioState.posZ - instance->cameraLoc.Y;

	sm64_mario_tick(marioInstance->marioId,
		&marioInstance->marioInputs,
		&marioInstance->marioState,
		&marioInstance->marioGeometry,
		&marioInstance->marioBodyState,
		true,
		true);

	auto carVelocity = car.GetVelocity();
	auto netVelocity = Vector(marioInstance->marioState.velX - carVelocity.X,
		marioInstance->marioState.velZ - carVelocity.Y,
		marioInstance->marioState.velY - carVelocity.Z);
	auto netPosition = Vector(marioInstance->marioState.posX - carPosition.X,
		marioInstance->marioState.posZ - carPosition.Y,
		marioInstance->marioState.posY - carPosition.Z);
	instance->marioAudio->UpdateSounds(marioInstance->marioState.soundMask,
		netPosition.X / 100.0f, netPosition.Y / 100.0f, netPosition.Z / 100.0f);

	if (marioInstance->marioGeometry.numTrianglesUsed > 0)
	{
		unsigned char* bodyStateBytes = (unsigned char*)&marioInstance->marioBodyState;
		unsigned char* marioStateBytes = (unsigned char*)&marioInstance->marioBodyState.marioState;
		std::string bodyStateStr = "B";
		bodyStateStr += instance->bytesToHex(bodyStateBytes, sizeof(struct SM64MarioBodyState) - sizeof(struct SM64MarioState));
		std::string marioStateStr = "M";
		marioStateStr += instance->bytesToHex(marioStateBytes, sizeof(struct SM64MarioState));

		if (bodyStateStr.length() < 110)
		{
			instance->Netcode->SendNewMessage(bodyStateStr);
		}
		if (marioStateStr.length() < 110)
		{
			instance->Netcode->SendNewMessage(marioStateStr);
		}
	}
}

#define WINGCAP_VERTEX_INDEX 750
void SM64::OnRender(CanvasWrapper canvas)
{
	if (renderer == nullptr) return;
	if (!meshesInitialized)
	{
		if (!renderer->Initialized) return;

		ballMesh = renderer->CreateMesh(utils.GetBakkesmodFolderPath() + "data\\assets\\ROCKETBALL.obj");
		
		if (ballMesh == nullptr) return;

		auto tmpTexture = (uint8_t*)malloc(SM64_TEXTURE_SIZE);
		memcpy(tmpTexture, texture, SM64_TEXTURE_SIZE);
		marioMesh = renderer->CreateMesh(SM64_GEO_MAX_TRIANGLES,
			texture,
			4 * SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT,
			SM64_TEXTURE_WIDTH,
			SM64_TEXTURE_HEIGHT);

		meshesInitialized = true;
		return;
	}

	auto inGame = gameWrapper->IsInGame() || gameWrapper->IsInReplay();
	if ((!renderLocalMario && !renderRemoteMario) || (!inGame && isActive))
	{
		marioMesh->RenderUpdateVertices(0, nullptr);
		return;
	}

	auto camera = gameWrapper->GetCamera();
	if (camera.IsNull()) return;

	auto server = gameWrapper->GetGameEventAsServer();
	if (server.IsNull())
	{
		server = gameWrapper->GetCurrentGameState();
	}

	if (server.IsNull()) return;

	auto localCar = gameWrapper->GetLocalCar();
	std::string localPlayerName = "";
	if (!localCar.IsNull())
	{
		auto localPlayer = localCar.GetPRI();
		if (!localPlayer.IsNull())
		{
			localPlayerName = localPlayer.GetPlayerName().ToString();
		}
	}

	for (CarWrapper car : server.GetCars())
	{
		auto player = car.GetPRI();
		if (player.IsNull()) continue;
		auto playerNamePtr = player.GetPlayerName();
		if (playerNamePtr.IsNull()) continue;
		auto playerName = playerNamePtr.ToString();

		SM64MarioInstance* marioInstance = nullptr;

		bool isLocalPlayer = false;
		bool isMatchAdmin = player.GetbMatchAdmin();
		if (playerName == localPlayerName)
		{
			// This is the local player - use that mario
			marioInstance = &localMario;
			isLocalPlayer = true;
		}
		else
		{
			// Not the local player. See if we have this player already
			if (remoteMarios.count(playerName) > 0)
			{
				marioInstance = remoteMarios[playerName];
			}
			else
			{
				// There is no mario initialized for this player
				continue;
			}
		}

		if (marioInstance == nullptr) continue;

		if (marioInstance->marioId < 0 && !isLocalPlayer)
		{
			marioInstance->marioId = sm64_mario_create((int16_t)marioInstance->marioState.posX,
				(int16_t)marioInstance->marioState.posY,
				(int16_t)marioInstance->marioState.posZ);
		}
		else if (marioInstance->marioId < 0 && isLocalPlayer)
		{
			carLocation = car.GetLocation();
			auto x = (int16_t)(carLocation.X);
			auto y = (int16_t)(carLocation.Y);
			auto z = (int16_t)(carLocation.Z);
			marioInstance->marioId = sm64_mario_create(x, z, y);
		}
		
		if (marioInstance->mesh == nullptr)
		{
			// Initialize the mesh
			auto tmpTexture = (uint8_t*)malloc(SM64_TEXTURE_SIZE);
			memcpy(tmpTexture, texture, SM64_TEXTURE_SIZE);
			marioInstance->mesh = renderer->CreateMesh(SM64_GEO_MAX_TRIANGLES,
				tmpTexture,
				4 * SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT,
				SM64_TEXTURE_WIDTH,
				SM64_TEXTURE_HEIGHT);
		}

		if (!isLocalPlayer)
		{
			sm64_mario_tick(marioInstance->marioId,
				&marioInstance->marioInputs,
				&marioInstance->marioBodyState.marioState,
				&marioInstance->marioGeometry,
				&marioInstance->marioBodyState,
				false,
				false);
		}
		else if(!isMatchAdmin)
		{
			tickMarioInstance(marioInstance, car, this);
		}

		for (auto i = 0; i < marioInstance->marioGeometry.numTrianglesUsed * 3; i++)
		{
			auto position = &marioInstance->marioGeometry.position[i * 3];
			auto color = &marioInstance->marioGeometry.color[i * 3];
			auto uv = &marioInstance->marioGeometry.uv[i * 2];
			auto normal = &marioInstance->marioGeometry.normal[i * 3];

			auto currentVertex = &marioInstance->mesh->Vertices[i];
			// Unreal engine swaps x and y coords for 3d model
			currentVertex->pos.x = position[0];
			currentVertex->pos.y = position[2];
			currentVertex->pos.z = position[1];
			currentVertex->color.x = color[0];
			currentVertex->color.y = color[1];
			currentVertex->color.z = color[2];
			currentVertex->color.w = i >= (WINGCAP_VERTEX_INDEX * 3) ? 0.0f : 1.0f;
			currentVertex->texCoord.x = uv[0];
			currentVertex->texCoord.y = uv[1];
			currentVertex->normal.x = normal[0];
			currentVertex->normal.y = normal[2];
			currentVertex->normal.z = normal[1];
		}

		marioInstance->mesh->RenderUpdateVertices(marioInstance->marioGeometry.numTrianglesUsed, &camera);

	}

	if (ballMesh != nullptr)
	{
		if (!server.IsNull())
		{
			auto ball = server.GetBall();
			if (!ball.IsNull())
			{
				auto ballLocation = ball.GetLocation();
				auto ballRotation = ball.GetRotation();
				auto quat = RotatorToQuat(ballRotation);

				ballMesh->SetRotationQuat(quat.X, quat.Y, quat.Z, quat.W);
				ballMesh->SetTranslation(ballLocation.X, ballLocation.Y, ballLocation.Z);
				ballMesh->SetScale(BALL_MODEL_SCALE, BALL_MODEL_SCALE, BALL_MODEL_SCALE);
				ballMesh->Render(&camera);

			}

		}
	}


}

SM64MarioInstance::SM64MarioInstance()
{
	marioGeometry.position = (float*)malloc(sizeof(float) * 9 * SM64_GEO_MAX_TRIANGLES);
	marioGeometry.color = (float*)malloc(sizeof(float) * 9 * SM64_GEO_MAX_TRIANGLES);
	marioGeometry.normal = (float*)malloc(sizeof(float) * 9 * SM64_GEO_MAX_TRIANGLES);
	marioGeometry.uv = (float*)malloc(sizeof(float) * 6 * SM64_GEO_MAX_TRIANGLES);
}

SM64MarioInstance::~SM64MarioInstance()
{
	free(marioGeometry.position);
	free(marioGeometry.color);
	free(marioGeometry.normal);
	free(marioGeometry.uv);
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

