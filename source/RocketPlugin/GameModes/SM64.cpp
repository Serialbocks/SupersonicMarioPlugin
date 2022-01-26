#include "SM64.h"

#include <thread>
#include <chrono>
#include <functional>

// CONSTANTS
#define RL_YAW_RANGE 64692

SM64::SM64(std::shared_ptr<GameWrapper> gw, std::shared_ptr<CVarManagerWrapper> cm, BakkesMod::Plugin::PluginInfo exports)
{
	using namespace std::placeholders;
	gameWrapper = gw;
	cvarManager = cm;
	Netcode = std::make_shared<NetcodeManager>(cvarManager, gameWrapper, exports,
		std::bind(&SM64::OnMessageReceived, this, _1, _2));

	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed", bind(&SM64::OnLeaveSession, this, _1));

	InitSM64();
	gameWrapper->RegisterDrawable(std::bind(&SM64::RenderMario, this, _1));

	typeIdx = std::make_unique<std::type_index>(typeid(*this));
}

SM64::~SM64()
{
	gameWrapper->UnregisterDrawables();
	DestroySM64();
}

void SM64::OnLeaveSession(std::string eventName)
{
	renderLocalMario = false;
	renderRemoteMario = false;
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
	std::string path = GetCurrentDirectory() + "\\baserom.us.z64";
	uint8_t* rom = utilsReadFileAlloc(path, &romSize);
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
	projectedVertices = (Renderer::MeshVertex*)malloc(sizeof(Renderer::MeshVertex) * 3 * SM64_GEO_MAX_TRIANGLES);
	ZeroMemory(projectedVertices, sizeof(Renderer::MeshVertex) * 3 * SM64_GEO_MAX_TRIANGLES);
	
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

	renderer = new Renderer(SM64_GEO_MAX_TRIANGLES,
		texture,
		textureSize,
		SM64_TEXTURE_WIDTH,
		SM64_TEXTURE_HEIGHT);
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
	free(projectedVertices);
	delete renderer;
}

void SM64::onTick(ServerWrapper server)
{
	for (CarWrapper car : server.GetCars())
	{
		PriWrapper player = car.GetPRI();
		if (!player.GetbMatchAdmin()) continue;

		if (marioId < 0)
		{
			Vector carLocation = car.GetLocation();
			auto x = (int16_t)(carLocation.X);
			auto y = (int16_t)(carLocation.Y);
			auto z = (int16_t)(carLocation.Z);

			carRotation = car.GetRotation();

			// Unreal swaps coords
			marioId = sm64_mario_create(x, z, y);
			if (marioId < 0) continue;
		}

		auto marioYaw = (int)(-marioState.faceAngle * (RL_YAW_RANGE / 6)) + (RL_YAW_RANGE / 4);
		car.SetLocation(Vector(marioState.posX, marioState.posZ, marioState.posY + carOffsetZ));
		car.SetVelocity(Vector(marioState.velX, marioState.velZ, marioState.velY + carOffsetZ));

		auto carRot = car.GetRotation();
		carRot.Yaw = marioYaw;
		carRot.Roll = carRotation.Roll;
		carRot.Pitch = carRotation.Pitch;
		car.SetRotation(carRot);
		
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

void SM64::RenderMario(CanvasWrapper canvas)
{
	if (!renderLocalMario && !renderRemoteMario) return;

	int inGame = (gameWrapper->IsInGame()) ? 1 : (gameWrapper->IsInReplay()) ? 2 : 0;
	//if (!inGame) return;

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

	if (renderRemoteMario)
	{
		sm64_mario_tick(remoteMarioId, &marioInputs, &marioBodyStateIn.marioState, &marioGeometry, &marioBodyStateIn, 0);
	}

	auto currentCameraLocation = camera.GetLocation();
	int triangleCount = 0;
	for (auto i = 0; i < marioGeometry.numTrianglesUsed; i++)
	{
		auto position = &marioGeometry.position[i * 9];
		auto color = &marioGeometry.color[i * 9];
		auto uv = &marioGeometry.uv[i * 6];

		// Unreal engine swaps x and y coords for 3d model
		auto tv1 = Vector(position[0], position[2], position[1] + marioOffsetZ);
		auto tv2 = Vector(position[3], position[5], position[4] + marioOffsetZ);
		auto tv3 = Vector(position[6], position[8], position[7] + marioOffsetZ);

		if (frust.IsInFrustum(tv1) || frust.IsInFrustum(tv2) || frust.IsInFrustum(tv3))
		{
			auto projV1 = canvas.ProjectF(tv1);
			auto projV2 = canvas.ProjectF(tv2);
			auto projV3 = canvas.ProjectF(tv3);

			auto distance1 = distance(currentCameraLocation, tv1) / depthFactor;
			auto distance2 = distance(currentCameraLocation, tv2) / depthFactor;
			auto distance3 = distance(currentCameraLocation, tv3) / depthFactor;

			auto baseVertexIndex = triangleCount * 3;
			projectedVertices[baseVertexIndex].position = Vector(projV1.X, projV1.Y, distance1);
			projectedVertices[baseVertexIndex].r = color[0];
			projectedVertices[baseVertexIndex].g = color[1];
			projectedVertices[baseVertexIndex].b = color[2];
			projectedVertices[baseVertexIndex].a = 1.0f;
			projectedVertices[baseVertexIndex].u = uv[0];
			projectedVertices[baseVertexIndex].v = uv[1];

			projectedVertices[baseVertexIndex + 1].position = Vector(projV2.X, projV2.Y, distance2);
			projectedVertices[baseVertexIndex + 1].r = color[3];
			projectedVertices[baseVertexIndex + 1].g = color[4];
			projectedVertices[baseVertexIndex + 1].b = color[5];
			projectedVertices[baseVertexIndex + 1].a = 1.0f;
			projectedVertices[baseVertexIndex + 1].u = uv[2];
			projectedVertices[baseVertexIndex + 1].v = uv[3];

			projectedVertices[baseVertexIndex + 2].position = Vector(projV3.X, projV3.Y, distance3);
			projectedVertices[baseVertexIndex + 2].r = color[6];
			projectedVertices[baseVertexIndex + 2].g = color[7];
			projectedVertices[baseVertexIndex + 2].b = color[8];
			projectedVertices[baseVertexIndex + 2].a = 1.0f;
			projectedVertices[baseVertexIndex + 2].u = uv[4];
			projectedVertices[baseVertexIndex + 2].v = uv[5];

			triangleCount++;
		}
	}

	renderer->RenderMesh(
		projectedVertices,
		triangleCount
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

