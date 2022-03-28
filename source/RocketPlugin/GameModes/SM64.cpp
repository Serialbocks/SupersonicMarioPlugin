#include "SM64.h"

#include <thread>
#include <chrono>
#include <functional>
#include <WinUser.h>

// CONSTANTS
#define RL_YAW_RANGE 64692
#define CAR_OFFSET_Z 45.0f
#define BALL_MODEL_SCALE 5.35f
#define SM64_TEXTURE_SIZE (4 * SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT)
#define WINGCAP_VERTEX_INDEX 750
#define ATTACK_BOOST_DAMAGE 0.20f
#define GROUND_POUND_BALL_RADIUS 200.0f
#define GROUND_POUND_PINCH_VELOCITY 2708.0f
#define ATTACK_BALL_RADIUS 261.0f
#define KICK_BALL_VEL_HORIZ 583.0f
#define KICK_BALL_VEL_VERT 305.0f
#define PUNCH_BALL_VEL_HORIZ 1388.0f
#define PUNCH_BALL_VEL_VERT 250.0f
#define DIVE_BALL_VEL_HORIZ 639.0f
#define DIVE_BALL_VEL_VERT 166.6f

inline void tickMarioInstance(SM64MarioInstance* marioInstance,
	CarWrapper car,
	SM64* instance);

void MessageReceived(char* buf, int len);

SM64* self = nullptr;

SM64::SM64(std::shared_ptr<GameWrapper> gw, std::shared_ptr<CVarManagerWrapper> cm, BakkesMod::Plugin::PluginInfo exports)
{
	using namespace std::placeholders;
	gameWrapper = gw;
	cvarManager = cm;

	// Register callback to receiving TCP data from server/clients
	Networking::RegisterCallback(MessageReceived);

	InitSM64();
	gameWrapper->RegisterDrawable(std::bind(&SM64::OnRender, this, _1));
	gameWrapper->HookEventPost("Function TAGame.EngineShare_TA.EventPostPhysicsStep", bind(&SM64::moveCarToMario, this, _1));
	gameWrapper->HookEventPost("Function TAGame.NetworkInputBuffer_TA.ClientAckFrame", bind(&SM64::moveCarToMario, this, _1));
	gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.PostGoalScored.Tick", bind(&SM64::onGoalScored, this, _1));

	attackBoostDamage = ATTACK_BOOST_DAMAGE;

	groundPoundPinchVel = GROUND_POUND_PINCH_VELOCITY;
	attackBallRadius = ATTACK_BALL_RADIUS;
	kickBallVelHoriz = KICK_BALL_VEL_HORIZ;
	kickBallVelVert = KICK_BALL_VEL_VERT;
	punchBallVelHoriz = PUNCH_BALL_VEL_HORIZ;
	punchBallVelVert = PUNCH_BALL_VEL_VERT;
	diveBallVelHoriz = DIVE_BALL_VEL_HORIZ;
	diveBallVelVert = DIVE_BALL_VEL_VERT;

	bljSetup.bljState = SM64_BLJ_STATE_DISABLED;
	bljSetup.bljVel = 0;

	typeIdx = std::make_unique<std::type_index>(typeid(*this));

	HookEventWithCaller<CarWrapper>(
		vehicleInputCheck,
		[this](const CarWrapper& caller, void* params, const std::string&) {
			onSetVehicleInput(caller, params);
		});

	HookEventWithCaller<ServerWrapper>(
		initialCharacterSpawnCheck,
		[this](const ServerWrapper& caller, void* params, const std::string&) {
			onCharacterSpawn(caller);
		});
	HookEventWithCaller<ServerWrapper>(
		CharacterSpawnCheck,
		[this](const ServerWrapper& caller, void* params, const std::string&) {
			onCharacterSpawn(caller);
		});
	HookEventWithCaller<ServerWrapper>(
		endPreGameTickCheck,
		[this](const ServerWrapper& caller, void* params, const std::string&) {
			onCountdownEnd(caller);
		});
	HookEventWithCaller<ServerWrapper>(
		clientEndPreGameTickCheck,
		[this](const ServerWrapper& caller, void* params, const std::string&) {
			onCountdownEnd(caller);
		});
	HookEventWithCaller<ServerWrapper>(
		overtimeGameCheck,
		[this](const ServerWrapper& caller, void* params, const std::string&) {
			onOvertimeStart(caller);
		});

	self = this;

}

SM64::~SM64()
{
	gameWrapper->UnregisterDrawables();
	DestroySM64();
	UnhookEvent(vehicleInputCheck);
	UnhookEvent(initialCharacterSpawnCheck);
	UnhookEvent(CharacterSpawnCheck);
	UnhookEvent(preGameTickCheck);
	UnhookEvent(endPreGameTickCheck);
	UnhookEvent(clientEndPreGameTickCheck);
	UnhookEvent(overtimeGameCheck);

}

void SM64::OnGameLeft()
{
	// Cleanup after a game session has been left
	isInSm64GameSema.acquire();
	isInSm64Game = false;
	isInSm64GameSema.release();
	if (localMario.mesh != nullptr)
	{
		localMario.mesh->RenderUpdateVertices(0, nullptr);
	}

	if (localMario.marioId >= 0)
	{
		sm64_mario_delete(localMario.marioId);
		localMario.marioId = -2;
		if (localMario.mesh != nullptr)
		{
			localMario.mesh->RenderUpdateVertices(0, nullptr);
			addMeshToPool(localMario.mesh);
			localMario.mesh = nullptr;
		}
	}
	for (const auto& remoteMario : remoteMarios)
	{
		SM64MarioInstance* marioInstance = remoteMario.second;
		marioInstance->sema.acquire();
		if (marioInstance->mesh != nullptr)
		{
			marioInstance->mesh->RenderUpdateVertices(0, nullptr);
			addMeshToPool(marioInstance->mesh);
			marioInstance->mesh = nullptr;
		}
		if (marioInstance->marioId >= 0)
		{
			sm64_mario_delete(marioInstance->marioId);
			marioInstance->marioId = -2;
		}
		marioInstance->sema.release();
	}
	remoteMarios.clear();
}

void SM64::moveCarToMario(std::string eventName)
{
	auto inGame = gameWrapper->IsInGame() || gameWrapper->IsInReplay() || gameWrapper->IsInOnlineGame();
	if (!inGame || isHost)
	{
		return;
	}
	isInSm64GameSema.acquire();
	bool inSm64Game = isInSm64Game;
	isInSm64GameSema.release();
	if (!inSm64Game)
	{
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

	auto car = gameWrapper->GetLocalCar();
	if (car.IsNull()) return;

	auto marioInstance = &localMario;

	marioInstance->sema.acquire();

	if (marioInstance->marioId >= 0)
	{
		auto marioState = &marioInstance->marioBodyState.marioState;
		auto marioYaw = (int)(-marioState->faceAngle * (RL_YAW_RANGE / 6)) + (RL_YAW_RANGE / 4);
		auto carPosition = Vector(marioState->posX, marioState->posZ, marioState->posY + CAR_OFFSET_Z);
		car.SetLocation(carPosition);
		car.SetVelocity(Vector(marioState->velX, marioState->velZ, marioState->velY));
		auto carRot = car.GetRotation();
		carRot.Yaw = marioYaw;
		carRot.Roll = carRotation.Roll;
		carRot.Pitch = carRotation.Pitch;
		car.SetRotation(carRot);
	}

	marioInstance->sema.release();
}

void SM64::onGoalScored(std::string eventName)
{
	isInSm64GameSema.acquire();
	bool inSm64Game = isInSm64Game;
	isInSm64GameSema.release();
	if (isInSm64Game)
	{
		OnGameLeft();
	}
}

void MessageReceived(char* buf, int len)
{
	auto targetLen = sizeof(struct SM64MarioBodyState) + sizeof(int);
	uint8_t* targetData = (uint8_t*)buf;
	if (len < targetLen)
	{
		return;
	}
	else if (len > targetLen)
	{
		targetData = (uint8_t*)(buf + len - targetLen);
	}

	int playerId = *((int*)buf);

	SM64MarioInstance* marioInstance = nullptr;
	self->remoteMariosSema.acquire();
	if (self->remoteMarios.count(playerId) == 0)
	{
		// Initialize mario for this player
		marioInstance = new SM64MarioInstance();
		self->remoteMarios[playerId] = marioInstance;
	}
	else
	{
		marioInstance = self->remoteMarios[playerId];
	}
	self->remoteMariosSema.release();

	marioInstance->sema.acquire();
	memcpy(&marioInstance->marioBodyState, targetData + sizeof(int), targetLen - sizeof(int));
	marioInstance->sema.release();

	self->isInSm64GameSema.acquire();
	self->isInSm64Game = true;
	self->isInSm64GameSema.release();
}

#define MIN_LIGHT_COORD -6000.0f
#define MAX_LIGHT_COORD 6000.0f
static int currentLightIndex = 0;
/// <summary>Renders the available options for the game mode.</summary>
void SM64::RenderOptions()
{
	if (renderer != nullptr)
	{
		ImGui::SliderFloat("Attack Boost Damage", &attackBoostDamage, 0.0f, 1.0f);

		ImGui::SliderFloat("Ground Pound Pinch Velocity", &groundPoundPinchVel, 0.0f, 15000.0f);
		ImGui::SliderFloat("Attack Ball Radius", &attackBallRadius, 0.0f, 600.0f);

		ImGui::SliderFloat("Kick Ball Vel Horiz", &kickBallVelHoriz, 0.0f, 10000.0f);
		ImGui::SliderFloat("Kick Ball Vel Vert", &kickBallVelVert, 0.0f, 10000.0f);

		ImGui::SliderFloat("Punch Ball Vel Horiz", &punchBallVelHoriz, 0.0f, 10000.0f);
		ImGui::SliderFloat("Punch Ball Vel Vert", &punchBallVelVert, 0.0f, 10000.0f);

		ImGui::SliderFloat("Dive Ball Vel Horiz", &diveBallVelHoriz, 0.0f, 10000.0f);
		ImGui::SliderFloat("Dive Ball Vel Vert", &diveBallVelVert, 0.0f, 10000.0f);

		ImGui::NewLine();

		ImGui::Text("BLJ Configuration");
		std::string bljLabel[3];
		bljLabel[0] = "BLJ Disabled";
		bljLabel[1] = "BLJ Enabled (Press)";
		bljLabel[2] = "BLJ Enabled (Hold)";
		std::string currentBljLabel = bljLabel[bljSetup.bljState];
		if (ImGui::BeginCombo("BLJ Mode Select", currentBljLabel.c_str()))
		{
			for (int i = 0; i < 3; i++)
			{
				bool isSelected = i == bljSetup.bljState;
				if (ImGui::Selectable(bljLabel[i].c_str(), isSelected))
					bljSetup.bljState = (SM64BljState)i;
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		if( 0 != bljSetup.bljState )
		{
			int velocity = bljSetup.bljVel;
			ImGui::SliderInt("BLJ Velocity", &velocity, 0, 10);
			bljSetup.bljVel = (uint8_t)velocity;
		}

		ImGui::NewLine();

		ImGui::Text("Cap Color");
		ImGui::SliderFloat("R", &testCapColorR, 0.0f, 1.0f);
		ImGui::SliderFloat("G", &testCapColorG, 0.0f, 1.0f);
		ImGui::SliderFloat("B", &testCapColorB, 0.0f, 1.0f);
		ImGui::Text("Shirt Color");
		ImGui::SliderFloat("Rs", &testShirtColorR, 0.0f, 1.0f);
		ImGui::SliderFloat("Gs", &testShirtColorG, 0.0f, 1.0f);
		ImGui::SliderFloat("Bs", &testShirtColorB, 0.0f, 1.0f);

		//ImGui::Text("Ambient Light");
		//ImGui::SliderFloat("R", &renderer->Lighting.AmbientLightColorR, 0.0f, 1.0f);
		//ImGui::SliderFloat("G", &renderer->Lighting.AmbientLightColorG, 0.0f, 1.0f);
		//ImGui::SliderFloat("B", &renderer->Lighting.AmbientLightColorB, 0.0f, 1.0f);
		//ImGui::SliderFloat("Ambient Strength", &renderer->Lighting.AmbientLightStrength, 0.0f, 1.0f);
		//
		//ImGui::NewLine();
		//
		//ImGui::Text("Dynamic Light");
		//std::string currentLightLabel = "Light " + std::to_string(currentLightIndex + 1);
		//if (ImGui::BeginCombo("Light Select", currentLightLabel.c_str()))
		//{
		//	for (int i = 0; i < MAX_LIGHTS; i++)
		//	{
		//		std::string lightLabel = "Light " + std::to_string(i + 1);
		//		bool isSelected = i == currentLightIndex;
		//		if (ImGui::Selectable(lightLabel.c_str(), isSelected))
		//			currentLightIndex = i;
		//		if (isSelected)
		//			ImGui::SetItemDefaultFocus();
		//	}
		//	ImGui::EndCombo();
		//}
		//
		//ImGui::SliderFloat("X", &renderer->Lighting.Lights[currentLightIndex].posX, MIN_LIGHT_COORD, MAX_LIGHT_COORD);
		//ImGui::SliderFloat("Y", &renderer->Lighting.Lights[currentLightIndex].posY, MIN_LIGHT_COORD, MAX_LIGHT_COORD);
		//ImGui::SliderFloat("Z", &renderer->Lighting.Lights[currentLightIndex].posZ, MIN_LIGHT_COORD, MAX_LIGHT_COORD);
		//ImGui::SliderFloat("Rd", &renderer->Lighting.Lights[currentLightIndex].r, 0.0f, 1.0f);
		//ImGui::SliderFloat("Gd", &renderer->Lighting.Lights[currentLightIndex].g, 0.0f, 1.0f);
		//ImGui::SliderFloat("Bd", &renderer->Lighting.Lights[currentLightIndex].b, 0.0f, 1.0f);
		//ImGui::SliderFloat("Dynamic Strength", &renderer->Lighting.Lights[currentLightIndex].strength, 0.0f, 1.0f);
		//ImGui::Checkbox("Show Bulb", &renderer->Lighting.Lights[currentLightIndex].showBulb);
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
		isHost = true;
		HookEventWithCaller<ServerWrapper>(
			gameTickCheck,
			[this](const ServerWrapper& caller, void* params, const std::string&) {
				onTick(caller);
			});


	}
	else if (!active && isActive) {
		isHost = false;

		UnhookEvent(gameTickCheck);
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

void SM64::onSetVehicleInput(CarWrapper car, void* params)
{
	PriWrapper player = car.GetPRI();
	if (player.IsNull()) return;
	auto isLocalPlayer = player.IsLocalPlayerPRI();

	auto inGame = gameWrapper->IsInGame() || gameWrapper->IsInReplay() || gameWrapper->IsInOnlineGame();
	isInSm64GameSema.acquire();
	bool inSm64Game = isInSm64Game;
	isInSm64GameSema.release();
	if (!inGame || !isInSm64Game)
	{
		return;
	}

	auto playerId = player.GetPlayerID();

	if (isHost || isLocalPlayer)
	{
		SM64MarioInstance* marioInstance = nullptr;

		if (isLocalPlayer)
		{
			marioInstance = &localMario;
			auto boostComponent = car.GetBoostComponent();
			if (!boostComponent.IsNull())
			{
				currentBoostAount = car.GetBoostComponent().GetCurrentBoostAmount();
			}
		}
		else
		{
			remoteMariosSema.acquire();
			if (remoteMarios.count(playerId) > 0)
			{
				marioInstance = remoteMarios[playerId];
			}
			remoteMariosSema.release();
		}

		if (marioInstance == nullptr) return;

		marioInstance->sema.acquire();

		if (marioInstance->marioId >= 0)
		{
			car.SetHidden2(TRUE);
			car.SetbHiddenSelf(TRUE);
			auto marioState = &marioInstance->marioBodyState.marioState;
			auto marioYaw = (int)(-marioState->faceAngle * (RL_YAW_RANGE / 6)) + (RL_YAW_RANGE / 4);
			auto carPosition = Vector(marioState->posX, marioState->posZ, marioState->posY + CAR_OFFSET_Z);
			car.SetLocation(carPosition);
			car.SetVelocity(Vector(marioState->velX, marioState->velZ, marioState->velY));
			auto carRot = car.GetRotation();
			carRot.Yaw = marioYaw;
			carRot.Roll = carRotation.Roll;
			carRot.Pitch = carRotation.Pitch;
			car.SetRotation(carRot);
			ControllerInput* newInput = (ControllerInput*)params;
			newInput->Jump = 0;
			newInput->Handbrake = 0;
			newInput->Throttle = 0;
			newInput->Steer = 0;
			newInput->Pitch = 0;

			// If attacked flag is set, decrement boost and demo if out of boost
			auto boostComponent = car.GetBoostComponent();
			if (marioInstance->marioBodyState.marioState.isAttacked && !boostComponent.IsNull())
			{
				marioInstance->marioInputs.attackInput.isAttacked = false;
				marioInstance->marioBodyState.marioState.isAttacked = false;
				float curBoostAmt = boostComponent.GetCurrentBoostAmount();
				if (curBoostAmt >= 0.01f)
				{
					curBoostAmt -= attackBoostDamage;
					curBoostAmt = curBoostAmt < 0 ? 0 : curBoostAmt;
				}
				boostComponent.SetCurrentBoostAmount(curBoostAmt);
				if (curBoostAmt < 0.01f)
				{
					car.Demolish();
				}
			}

			// Check if mario attacked ball and set velocity if so
			if (isHost)
			{
				auto server = gameWrapper->GetGameEventAsServer();
				if (server.IsNull())
				{
					server = gameWrapper->GetCurrentGameState();
				}

				if (!server.IsNull())
				{
					auto ball = server.GetBall();
					if (!ball.IsNull())
					{
						Vector marioVector(marioInstance->marioBodyState.marioState.posX,
							marioInstance->marioBodyState.marioState.posZ,
							marioInstance->marioBodyState.marioState.posY);
						Vector ballVector = ball.GetLocation();
						Vector ballVelocity = ball.GetVelocity();
						float distance = utils.Distance(marioVector, ballVector);

						float dx = ballVector.X - marioVector.X;
						float dy = ballVector.Y - marioVector.Y;

						bool attackedRecently = marioInstance->tickCount - marioInstance->lastBallInteraction < 10;

						float angleToBall = atan2f(dy, dx);
						if (attackedRecently)
						{
							// do nothing
						}
						else if (distance < GROUND_POUND_BALL_RADIUS &&
							marioInstance->marioBodyState.action == ACT_GROUND_POUND_LAND)
						{
							ballVelocity.X += groundPoundPinchVel * cosf(angleToBall);
							ballVelocity.Y += groundPoundPinchVel * sinf(angleToBall);
							ball.SetVelocity(ballVelocity);
							marioInstance->lastBallInteraction = marioInstance->tickCount;
						}
						else if (marioInstance->marioBodyState.action == ACT_JUMP_KICK &&
							distance < attackBallRadius)
						{
							ballVelocity.X += kickBallVelHoriz * cosf(angleToBall);
							ballVelocity.Y += kickBallVelHoriz * sinf(angleToBall);
							ballVelocity.Z += kickBallVelVert;
							ball.SetVelocity(ballVelocity);
							marioInstance->lastBallInteraction = marioInstance->tickCount;
						}
						else if (marioInstance->marioBodyState.action == ACT_MOVE_PUNCHING &&
							distance < attackBallRadius)
						{
							ballVelocity.X += punchBallVelHoriz * cosf(angleToBall);
							ballVelocity.Y += punchBallVelHoriz * sinf(angleToBall);
							ballVelocity.Z += punchBallVelVert;
							ball.SetVelocity(ballVelocity);
							marioInstance->lastBallInteraction = marioInstance->tickCount;
						}
						else if ((marioInstance->marioBodyState.action == ACT_DIVE || marioInstance->marioBodyState.action == ACT_DIVE_SLIDE) &&
							distance < attackBallRadius)
						{
							ballVelocity.X += diveBallVelHoriz * cosf(angleToBall);
							ballVelocity.Y += diveBallVelHoriz * sinf(angleToBall);
							ballVelocity.Z += diveBallVelVert;
							ball.SetVelocity(ballVelocity);
							marioInstance->lastBallInteraction = marioInstance->tickCount;
						}


						marioInstance->tickCount++;
					}
				}

			}

		}

		marioInstance->sema.release();

	}

}

void SM64::onCharacterSpawn(ServerWrapper server)
{
	HookEventWithCaller<ServerWrapper>(
		preGameTickCheck,
		[this](const ServerWrapper& caller, void* params, const std::string&) {
			onTick(caller);
		});
	isPreGame = true;
}

void SM64::onCountdownEnd(ServerWrapper server)
{
	UnhookEvent(preGameTickCheck);
	isPreGame = false;
}

void SM64::onTick(ServerWrapper server)
{
	if (!isHost) return;
	isInSm64GameSema.acquire();
	isInSm64Game = true;
	isInSm64GameSema.release();
	for (CarWrapper car : server.GetCars())
	{
		PriWrapper player = car.GetPRI();
		if (player.IsNull()) continue;
		if (player.IsLocalPlayerPRI())
		{
			remoteMariosSema.acquire();
			tickMarioInstance(&localMario, car, this);
			remoteMariosSema.release();
		}

	}
}

void SM64::onOvertimeStart(ServerWrapper server)
{
	isInSm64GameSema.acquire();
	bool inSm64Game = isInSm64Game;
	isInSm64GameSema.release();
	if (isInSm64Game)
	{
		OnGameLeft();
	}
}

inline void tickMarioInstance(SM64MarioInstance* marioInstance,
	CarWrapper car,
	SM64* instance)
{
	if (car.IsNull()) return;
	instance->carLocation = car.GetLocation();
	auto x = (int16_t)(instance->carLocation.X);
	auto y = (int16_t)(instance->carLocation.Y);
	auto z = (int16_t)(instance->carLocation.Z);

	marioInstance->sema.acquire();
	if (marioInstance->marioId < 0)
	{
		// Unreal swaps coords
		instance->carRotation = car.GetRotation();
		marioInstance->marioId = sm64_mario_create(x, z, y);
		if (marioInstance->marioId < 0) return;
	}

	auto camera = instance->gameWrapper->GetCamera();
	if (!camera.IsNull())
	{
		instance->cameraLoc = camera.GetLocation();
	}

	auto playerController = car.GetPlayerController();
	if (!instance->isPreGame && !playerController.IsNull())
	{
		auto playerInputs = playerController.GetVehicleInput();
		marioInstance->marioInputs.buttonA = playerInputs.Jump;
		marioInstance->marioInputs.buttonB = playerInputs.Handbrake;
		marioInstance->marioInputs.buttonZ = playerInputs.Throttle < 0;
		marioInstance->marioInputs.stickX = playerInputs.Steer;
		marioInstance->marioInputs.stickY = playerInputs.Pitch;
	}
	else
	{
		marioInstance->marioInputs.buttonA = NULL;
		marioInstance->marioInputs.buttonB = NULL;
		marioInstance->marioInputs.buttonZ = NULL;
		marioInstance->marioInputs.stickX = NULL;
		marioInstance->marioInputs.stickY = NULL;
	}

	marioInstance->marioInputs.camLookX = marioInstance->marioState.posX - instance->cameraLoc.X;
	marioInstance->marioInputs.camLookZ = marioInstance->marioState.posZ - instance->cameraLoc.Y;

	auto controllerInput = car.GetPlayerController().GetVehicleInput();
	marioInstance->marioInputs.isBoosting = controllerInput.HoldingBoost && instance->currentBoostAount >= 0.01f;

	// Determine interaction between other marios
	marioInstance->marioInputs.attackInput.isAttacked = false;
	marioInstance->marioInputs.attackInput.attackedPosX = 0;
	marioInstance->marioInputs.attackInput.attackedPosY = 0;
	marioInstance->marioInputs.attackInput.attackedPosZ = 0;
	if (marioInstance->marioId >= 0)
	{
		Vector localMarioVector(marioInstance->marioState.posX,
			marioInstance->marioState.posY,
			marioInstance->marioState.posZ);
		for (auto const& [playerId, remoteMarioInstance] : instance->remoteMarios)
		{
			if (marioInstance->marioInputs.attackInput.isAttacked)
				break;

			if (remoteMarioInstance->marioId < 0)
				continue;

			remoteMarioInstance->sema.acquire();

			Vector remoteMarioVector(remoteMarioInstance->marioBodyState.marioState.posX,
				remoteMarioInstance->marioBodyState.marioState.posY,
				remoteMarioInstance->marioBodyState.marioState.posZ);

			auto distance = instance->utils.Distance(localMarioVector, remoteMarioVector);
			if (distance < 100.0f && remoteMarioInstance->marioBodyState.action & ACT_FLAG_ATTACKING) // TODO if in our direction
			{
				marioInstance->marioInputs.attackInput.isAttacked = true;
				marioInstance->marioInputs.attackInput.attackedPosX = remoteMarioVector.X;
				marioInstance->marioInputs.attackInput.attackedPosY = remoteMarioVector.Y;
				marioInstance->marioInputs.attackInput.attackedPosZ = remoteMarioVector.Z;
			}
			remoteMarioInstance->sema.release();
		}
	}

	marioInstance->marioInputs.bljInput =  instance->bljSetup;

	sm64_mario_tick(marioInstance->marioId,
		&marioInstance->marioInputs,
		&marioInstance->marioState,
		&marioInstance->marioGeometry,
		&marioInstance->marioBodyState,
		true,
		true);


	auto marioVector = Vector(marioInstance->marioState.posX, marioInstance->marioState.posZ, marioInstance->marioState.posY);
	auto marioVel = Vector(marioInstance->marioState.velX, marioInstance->marioState.velZ, marioInstance->marioState.velY);
	auto quat = RotatorToQuat(camera.GetRotation());
	instance->cameraLoc = camera.GetLocation();
	Vector cameraAt = RotateVectorWithQuat(Vector(1, 0, 0), quat);

	marioInstance->slidingHandle = instance->marioAudio->UpdateSounds(marioInstance->marioState.soundMask,
		marioVector,
		marioVel,
		instance->cameraLoc,
		cameraAt,
		marioInstance->slidingHandle,
		marioInstance->marioBodyState.action);

	int playerId = car.GetPRI().GetPlayerID();
	memcpy(self->netcodeOutBuf, &playerId, sizeof(int));
	memcpy(self->netcodeOutBuf + sizeof(int), &marioInstance->marioBodyState, sizeof(struct SM64MarioBodyState));
	Networking::SendBytes(self->netcodeOutBuf, sizeof(struct SM64MarioBodyState) + sizeof(int));
	marioInstance->sema.release();
}

void backgroundLoadData()
{
	self->utils.ParseObjFile(self->utils.GetBakkesmodFolderPath() + "data\\assets\\ROCKETBALL.obj", &self->ballVertices);
	self->backgroundLoadThreadFinished = true;
	self->marioAudio->CheckAndModulateSounds();
}

inline void renderMario(SM64MarioInstance* marioInstance, CameraWrapper camera)
{
	if (marioInstance == nullptr) return;

	marioInstance->sema.acquire();
	if (marioInstance->mesh != nullptr)
	{
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
	marioInstance->sema.release();
}

void SM64::OnRender(CanvasWrapper canvas)
{
	if (renderer == nullptr) return;
	if (!backgroundLoadThreadStarted)
	{
		if (!renderer->Initialized) return;
		std::thread loadMeshesThread(backgroundLoadData);
		loadMeshesThread.detach();
		backgroundLoadThreadStarted = true;
		return;
	}

	if (!backgroundLoadThreadFinished) return;

	if (!meshesInitialized)
	{
		ballMesh = renderer->CreateMesh(&ballVertices);

		marioMeshPoolSema.acquire();
		for (int i = 0; i < MARIO_MESH_POOL_SIZE; i++)
		{
			marioMeshPool.push_back(renderer->CreateMesh(SM64_GEO_MAX_TRIANGLES,
				texture,
				4 * SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT,
				SM64_TEXTURE_WIDTH,
				SM64_TEXTURE_HEIGHT));
		}
		marioMeshPoolSema.release();

		meshesInitialized = true;
	}

	auto inGame = gameWrapper->IsInGame() || gameWrapper->IsInReplay() || gameWrapper->IsInOnlineGame();
	isInSm64GameSema.acquire();
	bool inSm64Game = isInSm64Game;
	isInSm64GameSema.release();
	if (!inGame || !isInSm64Game)
	{
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

	remoteMariosSema.acquire();
	for (auto const& [playerId, marioInstance] : remoteMarios)
	{
		marioInstance->CarActive = false;
	}

	localMario.CarActive = false;

	// Render local mario, and mark which marios no longer exist
	int redTeamIndex = 0;
	int blueTeamIndex = 0;
	for (CarWrapper car : server.GetCars())
	{
		auto player = car.GetPRI();
		if (player.IsNull()) continue;
		auto playerName = player.GetPlayerName().ToString();

		int teamIndex = -1;

		auto team = player.GetTeam();
		if (!team.IsNull())
		{
			teamIndex = team.GetTeamIndex();
		}

		// Determine which colors should apply to this mario
		std::vector<float> teamColors = blueTeamColors;
		int colorIndex = 0;
		if (teamIndex == 1)
		{
			teamColors = redTeamColors;
			colorIndex = redTeamIndex * 6;
			redTeamIndex++;
		}
		else
		{
			colorIndex = blueTeamIndex * 6;
			blueTeamIndex++;
		}

		auto playerId = player.GetPlayerID();
		if (remoteMarios.count(playerId) > 0)
		{
			auto remoteMario = remoteMarios[playerId];
			remoteMario->CarActive = true;
			if (remoteMario->mesh != nullptr)
			{

				remoteMario->mesh->SetCapColor(teamColors[colorIndex],
					teamColors[colorIndex+1],
					teamColors[colorIndex+2]);
				remoteMario->mesh->SetShirtColor(teamColors[colorIndex+3],
					teamColors[colorIndex+4],
					teamColors[colorIndex+5]);
				//remoteMario->mesh->ShowNameplate(L"", car.GetLocation());
			}
		}

		if (playerName != localPlayerName)
		{
			continue;
		}

		localMario.CarActive = true;

		SM64MarioInstance* marioInstance = &localMario;


		carLocation = car.GetLocation();

		if (marioInstance->mesh == nullptr)
		{
			marioInstance->mesh = getMeshFromPool();
		}
		else
		{
			marioInstance->mesh->SetCapColor(teamColors[colorIndex],
				teamColors[colorIndex + 1],
				teamColors[colorIndex + 2]);
			marioInstance->mesh->SetShirtColor(teamColors[colorIndex + 3],
				teamColors[colorIndex + 4],
				teamColors[colorIndex + 5]);
			//marioInstance->mesh->ShowNameplate(L"", car.GetLocation());
		}

		if(!isHost)
		{
			tickMarioInstance(marioInstance, car, this);
		}

		renderMario(marioInstance, camera);
	}

	if (!localMario.CarActive)
	{
		if (localMario.marioId >= 0)
		{
			sm64_mario_delete(localMario.marioId);
			localMario.marioId = -2;
		}

		if (localMario.mesh != nullptr)
		{
			localMario.mesh->RenderUpdateVertices(0, nullptr);
		}
	}

	// Loop through remote marios and render
	for (auto const& [playerId, marioInstance] : remoteMarios)
	{
		marioInstance->sema.acquire();
		if (marioInstance->mesh == nullptr)
		{
			marioInstance->mesh = getMeshFromPool();
			if (marioInstance->mesh == nullptr)
			{
				marioInstance->sema.release();
				continue;
			}
		}
		if (marioInstance->marioId < 0)
		{
			marioInstance->marioId = sm64_mario_create((int16_t)marioInstance->marioState.posX,
				(int16_t)marioInstance->marioState.posY,
				(int16_t)marioInstance->marioState.posZ);
		}

		sm64_mario_tick(marioInstance->marioId,
			&marioInstance->marioInputs,
			&marioInstance->marioBodyState.marioState,
			&marioInstance->marioGeometry,
			&marioInstance->marioBodyState,
			false,
			false);

		auto marioVector = Vector(marioInstance->marioBodyState.marioState.posX,
			marioInstance->marioBodyState.marioState.posZ,
			marioInstance->marioBodyState.marioState.posY);
		auto marioVel = Vector(marioInstance->marioBodyState.marioState.velX,
			marioInstance->marioBodyState.marioState.velZ,
			marioInstance->marioBodyState.marioState.velY);
		auto quat = RotatorToQuat(camera.GetRotation());
		cameraLoc = camera.GetLocation();
		Vector cameraAt = RotateVectorWithQuat(Vector(1, 0, 0), quat);

		marioInstance->slidingHandle = marioAudio->UpdateSounds(marioInstance->marioBodyState.marioState.soundMask,
			marioVector,
			marioVel,
			cameraLoc,
			cameraAt,
			marioInstance->slidingHandle,
			marioInstance->marioBodyState.action);

		marioInstance->marioBodyState.marioState.soundMask = 0;

		if (!marioInstance->CarActive)
		{
			if (marioInstance->marioId >= 0)
			{
				sm64_mario_delete(marioInstance->marioId);
				marioInstance->marioId = -2;
			}

			if (marioInstance->mesh != nullptr)
			{
				marioInstance->mesh->RenderUpdateVertices(0, nullptr);
				addMeshToPool(marioInstance->mesh);
				marioInstance->mesh = nullptr;
			}
		}

		marioInstance->sema.release();

		renderMario(marioInstance, camera);
	}
	remoteMariosSema.release();


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

std::string SM64::bytesToHex(unsigned char* data, unsigned int len)
{
	std::stringstream ss;
	ss << std::hex << std::setfill('0');
	for (unsigned int i = 0; i < len; ++i)
		ss << std::setw(2) << static_cast<unsigned>(data[i]);

	return ss.str();
}

Mesh* SM64::getMeshFromPool()
{
	Mesh* mesh = nullptr;
	marioMeshPoolSema.acquire();
	if (marioMeshPool.size() > 0)
	{
		mesh = marioMeshPool[0];
		marioMeshPool.erase(marioMeshPool.begin());
	}
	marioMeshPoolSema.release();
	return mesh;
}

void SM64::addMeshToPool(Mesh* mesh)
{
	if (mesh != nullptr)
	{
		marioMeshPoolSema.acquire();
		marioMeshPool.push_back(mesh);
		marioMeshPoolSema.release();
	}
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


