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

	typeIdx = std::make_unique<std::type_index>(typeid(*this));
}

void SM64::OnMessageReceived(const std::string& message, PriWrapper sender)
{

}

void SM64::onLoad()
{
	
}

void SM64::onUnload()
{
	
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
		InitSM64();
		HookEventWithCaller<ServerWrapper>(
			"Function GameEvent_Soccar_TA.Active.Tick",
			[this](const ServerWrapper& caller, void* params, const std::string&) {
				onTick(caller);
			});
	}
	else if (!active && isActive) {
		UnhookEvent("Function GameEvent_Soccar_TA.Active.Tick");
		DestroySM64();
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

std::string hexStr(unsigned char* data, unsigned int len)
{
	std::stringstream ss;
	ss << std::hex << std::setfill('0');
	for (unsigned int i = 0; i < len; ++i)
		ss << std::setw(2) << static_cast<unsigned>(data[i]);

	return ss.str();
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
		
		PlayerControllerWrapper playerController = car.GetPlayerController();
		playerInputs = playerController.GetVehicleInput();
		marioInputs.buttonA = playerInputs.Jump;
		marioInputs.buttonB = playerInputs.Handbrake;
		marioInputs.buttonZ = playerInputs.Throttle < 0;
		marioInputs.stickX = playerInputs.Steer;
		marioInputs.stickY = playerInputs.Pitch;
		marioInputs.camLookX = marioState.posX;
		marioInputs.camLookZ = marioState.posZ;
		
		sm64_mario_tick(marioId, &marioInputs, &marioState, &marioGeometry, &marioBodyState, 1);

		if (marioGeometry.numTrianglesUsed > 0)
		{
			unsigned char* bodyStateBytes = (unsigned char*)&marioBodyState;
			unsigned char* marioStateBytes = (unsigned char*)&marioBodyState.marioState;
			std::string bodyStateStr = "B";
			bodyStateStr += hexStr(bodyStateBytes, sizeof(struct SM64MarioBodyState) - sizeof(struct SM64MarioState));
			std::string marioStateStr = "M";
			marioStateStr += hexStr(marioStateBytes, sizeof(struct SM64MarioState));

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


float SM64::distance(Vector v1, Vector v2)
{
	return (float)sqrt(pow(v2.X - v1.X, 2.0) + pow(v2.Y - v1.Y, 2.0) + pow(v2.Z - v1.Z, 2.0));
}

