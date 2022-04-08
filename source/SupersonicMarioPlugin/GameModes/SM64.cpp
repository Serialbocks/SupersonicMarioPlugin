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

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
void initAudio();
ma_device device;
bool audioInitialized = false;
SM64* self = nullptr;

SM64::SM64(std::shared_ptr<GameWrapper> gw, std::shared_ptr<CVarManagerWrapper> cm, BakkesMod::Plugin::PluginInfo exports)
{
	using namespace std::placeholders;
	gameWrapper = gw;
	cvarManager = cm;

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

	matchSettings.bljSetup.bljState = SM64_BLJ_STATE_DISABLED;
	matchSettings.bljSetup.bljVel = 0;

	typeIdx = std::make_unique<std::type_index>(typeid(*this));

	HookEventWithCaller<CarWrapper>(
		vehicleInputCheck,
		[this](const CarWrapper& caller, void* params, const std::string&) {
			onSetVehicleInput(caller, params);
		});

	HookEventWithCaller<ServerWrapper>(
		nameplateTickCheck,
		[this](const ServerWrapper& caller, void* params, const std::string&) {
			onNameplateTick(caller, params);
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
	HookEventWithCaller<ServerWrapper>(
		clientOvertimeGameCheck,
		[this](const ServerWrapper& caller, void* params, const std::string&) {
			onOvertimeStart(caller);
		});
	HookEventWithCaller<ServerWrapper>(
		playerLeaveOrJoinCheck,
		[this](const ServerWrapper& caller, void* params, const std::string&) {
			sendSettingsIfHost(caller);
		});
	HookEventWithCaller<ServerWrapper>(
		playerJoinedTeamCheck,
		[this](const ServerWrapper& caller, void* params, const std::string&) {
			sendSettingsIfHost(caller);
		});
	HookEventWithCaller<ServerWrapper>(
		playerLeftTeamCheck,
		[this](const ServerWrapper& caller, void* params, const std::string&) {
			sendSettingsIfHost(caller);
		});
	HookEventWithCaller<ServerWrapper>(
		menuPushCheck,
		[this](const ServerWrapper& caller, void* params, const std::string&) {
			menuPushed(caller);
		});
	HookEventWithCaller<ServerWrapper>(
		menuPopCheck,
		[this](const ServerWrapper& caller, void* params, const std::string&) {
			menuPopped(caller);
		});

	self = this;

	// Register callback to receiving TCP data from server/clients
	Networking::RegisterCallback(MessageReceived);
}

SM64::~SM64()
{
	
	gameWrapper->UnregisterDrawables();
	//UnhookEvent(vehicleInputCheck);
	//UnhookEvent(initialCharacterSpawnCheck);
	//UnhookEvent(CharacterSpawnCheck);
	//UnhookEvent(preGameTickCheck);
	//UnhookEvent(endPreGameTickCheck);
	//UnhookEvent(clientEndPreGameTickCheck);
	//UnhookEvent(overtimeGameCheck);
	//UnhookEvent(clientOvertimeGameCheck);
	gameWrapper->UnhookEventPost("Function TAGame.EngineShare_TA.EventPostPhysicsStep");
	gameWrapper->UnhookEventPost("Function TAGame.NetworkInputBuffer_TA.ClientAckFrame");
	gameWrapper->UnhookEventPost("Function GameEvent_Soccar_TA.PostGoalScored.Tick");

	remoteMariosSema.release();
	matchSettingsSema.release();
	marioMeshPoolSema.release();
	DestroySM64();
}

void SM64::OnGameLeft(bool deleteMario)
{
	// Cleanup after a game session has been left
	matchSettingsSema.acquire();
	matchSettings.isInSm64Game = false;
	matchSettingsSema.release();
	if (localMario.mesh != nullptr)
	{
		localMario.mesh->RenderUpdateVertices(0, nullptr);
	}

	if (localMario.marioId >= 0)
	{
		if (deleteMario)
		{
			sm64_mario_delete(localMario.marioId);
			localMario.marioId = -2;
		}
		else
		{
			localMario.marioBodyState.marioState.posX = 0.0f;
			localMario.marioBodyState.marioState.posY = 0.0f;
			localMario.marioBodyState.marioState.posZ = 0.0f;
		}

		if (localMario.mesh != nullptr)
		{
			localMario.mesh->RenderUpdateVertices(0, nullptr);
			if (deleteMario)
			{
				addMeshToPool(localMario.mesh);
				localMario.mesh = nullptr;
			}
		}
		if (isHost && deleteMario && localMario.colorIndex >= 0)
		{
			addColorIndexToPool(localMario.colorIndex);
			localMario.colorIndex = -1;
		}

	}

	remoteMariosSema.acquire();
	for (const auto& remoteMario : remoteMarios)
	{
		SM64MarioInstance* marioInstance = remoteMario.second;
		marioInstance->sema.acquire();
		if (marioInstance->mesh != nullptr)
		{
			marioInstance->mesh->RenderUpdateVertices(0, nullptr);
			if (deleteMario)
			{
				addMeshToPool(marioInstance->mesh);
				marioInstance->mesh = nullptr;
			}

		}
		if (deleteMario && marioInstance->marioId >= 0)
		{
			sm64_mario_delete(marioInstance->marioId);
			marioInstance->marioId = -2;
		}
		if (isHost && deleteMario && marioInstance->colorIndex >= 0)
		{
			addColorIndexToPool(marioInstance->colorIndex);
			marioInstance->colorIndex = -1;
		}
		if (!deleteMario)
		{
			marioInstance->marioBodyState.marioState.posX = 0.0f;
			marioInstance->marioBodyState.marioState.posY = 0.0f;
			marioInstance->marioBodyState.marioState.posZ = 0.0f;
		}
		marioInstance->sema.release();
	}
	if (deleteMario)
	{
		remoteMarios.clear();
		Activate(false);
	}
		
	remoteMariosSema.release();
	sm64_stop_continuous_sounds();
}

void SM64::moveCarToMario(std::string eventName)
{
	auto inGame = gameWrapper->IsInGame() || gameWrapper->IsInReplay() || gameWrapper->IsInOnlineGame();
	if (!inGame || isHost)
	{
		return;
	}
	matchSettingsSema.acquire();
	bool inSm64Game = matchSettings.isInSm64Game;
	matchSettingsSema.release();
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
		if( marioState->posX == 0 && marioState->posY == 0 && marioState->posZ == 0 )
		{
			marioInstance->sema.release();
			return;
		}
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


	if (!isHost)
	{
		remoteMariosSema.acquire();
		for (CarWrapper car : server.GetCars())
		{
			PriWrapper player = car.GetPRI();
			if (player.IsNull()) return;
			auto isLocalPlayer = player.IsLocalPlayerPRI();
			if (isLocalPlayer) continue;
			auto playerId = player.GetPlayerID();

			if (remoteMarios.count(playerId) > 0)
			{
				marioInstance = remoteMarios[playerId];
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
		}
		remoteMariosSema.release();
		
	}
}


void SM64::onGoalScored(std::string eventName)
{
	matchSettingsSema.acquire();
	bool inSm64Game = matchSettings.isInSm64Game;
	matchSettingsSema.release();
	if (inSm64Game)
	{
		OnGameLeft(false);
	}
}

void SM64::menuPushed(ServerWrapper server)
{
	menuStackCount++;
}

void SM64::menuPopped(ServerWrapper server)
{
	if(menuStackCount > 0)
		menuStackCount--;
}

void MarioMessageReceived(char* buf, int len)
{
	auto marioMsgLen = sizeof(struct SM64MarioBodyState) + sizeof(int);
	uint8_t* targetData = (uint8_t*)buf;
	if (len != marioMsgLen) return;

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
	memcpy(&marioInstance->marioBodyState, targetData + sizeof(int), marioMsgLen - sizeof(int));
	marioInstance->sema.release();

	self->matchSettingsSema.acquire();
	self->matchSettings.isInSm64Game = true;
	self->matchSettingsSema.release();
}

void MatchSettingsMessageReceived(char* buf, int len)
{
	auto settingsMsgLen = sizeof(MatchSettings) + sizeof(int);
	if (len != settingsMsgLen) return;

	self->matchSettingsSema.acquire();
	memcpy(&self->matchSettings, buf + sizeof(int), settingsMsgLen - sizeof(int));
	self->matchSettingsSema.release();

	// Sync player colors
	self->remoteMariosSema.acquire();
	self->localMario.sema.acquire();
	int localMarioPlayerId = self->localMario.playerId;
	self->localMario.sema.release();
	for (int i = 0; i < self->matchSettings.playerCount; i++)
	{
		int playerId = self->matchSettings.playerIds[i];
		int colorIndex = self->matchSettings.playerColorIndices[i];
		SM64MarioInstance* marioInstance = nullptr;
		if (self->remoteMarios.count(playerId) > 0)
		{
			marioInstance = self->remoteMarios[playerId];
		}
		else if (playerId == localMarioPlayerId)
		{
			marioInstance = &self->localMario;
		}

		if (marioInstance != nullptr)
		{
			marioInstance->sema.acquire();
			marioInstance->colorIndex = colorIndex;
			marioInstance->isStem = self->matchSettings.playerStemFlags[i];
			marioInstance->sema.release();
		}
	}
	self->remoteMariosSema.release();
}

void MessageReceived(char* buf, int len)
{
	if (len < sizeof(int)) return;
	int messageId = *((int*)buf);
	if (messageId == -1)
	{
		MatchSettingsMessageReceived(buf, len);
	}
	else
	{
		MarioMessageReceived(buf, len);
	}
}

// Assumes matchSettingsSema is already acquired. Don't acquire here!
void SM64::SendSettingsToClients()
{
	int messageId = -1;

	if (matchSettings.isInSm64Game)
	{
		matchSettings.playerCount = 0;
		localMario.sema.acquire();
		if (localMario.colorIndex >= 0)
		{
			matchSettings.playerIds[matchSettings.playerCount] = localMario.playerId;
			matchSettings.playerColorIndices[matchSettings.playerCount] = localMario.colorIndex;
			matchSettings.playerStemFlags[matchSettings.playerCount] = localMario.isStem;
			matchSettings.playerCount++;
		}
		localMario.sema.release();

		remoteMariosSema.acquire();
		for (auto const& [playerId, marioInstance] : remoteMarios)
		{
			marioInstance->sema.acquire();
			if (marioInstance->colorIndex >= 0)
			{
				matchSettings.playerIds[matchSettings.playerCount] = playerId;
				matchSettings.playerColorIndices[matchSettings.playerCount] = marioInstance->colorIndex;
				matchSettings.playerStemFlags[matchSettings.playerCount] = marioInstance->isStem;
				matchSettings.playerCount++;
			}
			marioInstance->sema.release();
		}
		remoteMariosSema.release();
	}

	memcpy(self->netcodeOutBuf, &messageId, sizeof(int));
	memcpy(self->netcodeOutBuf + sizeof(int), &matchSettings, sizeof(MatchSettings));
	Networking::SendBytes(self->netcodeOutBuf, sizeof(MatchSettings) + sizeof(int));
}

void SM64::sendSettingsIfHost(ServerWrapper server)
{
	if (isHost)
	{
		matchSettingsSema.acquire();
		SendSettingsToClients();
		matchSettingsSema.release();
	}
}

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
		matchSettingsSema.acquire();
		std::string currentBljLabel = bljLabel[matchSettings.bljSetup.bljState];
		if (ImGui::BeginCombo("BLJ Mode Select", currentBljLabel.c_str()))
		{
			for (int i = 0; i < 3; i++)
			{
				bool isSelected = i == matchSettings.bljSetup.bljState;
				if (ImGui::Selectable(bljLabel[i].c_str(), isSelected))
				{
					matchSettings.bljSetup.bljState = (SM64BljState)i;
					SendSettingsToClients();
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		if( 0 != matchSettings.bljSetup.bljState )
		{
			int velocity = matchSettings.bljSetup.bljVel;
			ImGui::SliderInt("BLJ Velocity", &velocity, 0, 10);
			matchSettings.bljSetup.bljVel = (uint8_t)velocity;
		}


		ImGui::NewLine();

		auto inGame = gameWrapper->IsInGame() || gameWrapper->IsInReplay() || gameWrapper->IsInOnlineGame();
		bool inSm64Game = matchSettings.isInSm64Game;
		bool needToSendMatchUpdate = false;
		if (inGame && inSm64Game)
		{
			auto server = gameWrapper->GetGameEventAsServer();
			if (server.IsNull())
			{
				server = gameWrapper->GetCurrentGameState();
				if (server.IsNull()) return;
			}

			remoteMariosSema.acquire();
			for (CarWrapper car : server.GetCars())
			{
				PriWrapper player = car.GetPRI();
				if (player.IsNull()) continue;
				int playerId = player.GetPlayerID();

				SM64MarioInstance* marioInstance = nullptr;
				if (localMario.playerId == playerId)
				{
					marioInstance = &localMario;
				}
				else if (remoteMarios.count(playerId) > 0)
				{
					marioInstance = remoteMarios[playerId];
				}

				if (marioInstance == nullptr) continue;

				std::string playerName = player.GetPlayerName().ToString() + " Stem Mode";

				marioInstance->sema.acquire();
				
				
				bool oldStem = marioInstance->isStem;
				ImGui::Checkbox(playerName.c_str(), &marioInstance->isStem);
				marioInstance->sema.release();

				if (oldStem != marioInstance->isStem)
				{
					needToSendMatchUpdate = true;
				}

			}
			remoteMariosSema.release();

		}

		if (needToSendMatchUpdate)
		{
			SendSettingsToClients();
		}

		matchSettingsSema.release();
	}
}

void SM64::RenderPreferences()
{
	if (audioInitialized)
	{
		int intVol = (int)(device.masterVolumeFactor * 100);
		ImGui::TextUnformatted("Preferences");
		ImGui::SliderInt("Mario Volume", &intVol, 0, 100);
		if (ImGui::IsItemDeactivatedAfterChange())
		{
			MarioConfig::getInstance().SetVolume(intVol);
		}
		device.masterVolumeFactor = intVol / 100.0f;
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

	initRom(romPath);
	if (rom == NULL)
	{
		return;
	}

	texture = (uint8_t*)malloc(SM64_TEXTURE_SIZE);
	stemTexture = (uint8_t*)malloc(SM64_TEXTURE_SIZE);

	//sm64_global_terminate();
	if (!sm64Initialized)
	{
		sm64_global_init(rom.get(), texture, stemTexture, NULL);
		sm64_static_surfaces_load(surfaces, surfaces_count);
		initAudio();
		sm64Initialized = true;
	}

	cameraPos[0] = 0.0f;
	cameraPos[1] = 0.0f;
	cameraPos[2] = 0.0f;
	cameraRot = 0.0f;

	locationInit = false;

	renderer = new Renderer();
}

void SM64::DestroySM64()
{
	if (localMario.marioId >= 0)
	{
		sm64_mario_delete(localMario.marioId);
	}
	localMario.marioId = -2;
	sm64_global_terminate();
	free(texture);
	free(stemTexture);
	for (int i = 0; i < remoteMarios.size(); i++)
	{
		remoteMarios[i]->sema.release();
		delete remoteMarios[i];
	}
	delete renderer;
}

void SM64::onSetVehicleInput(CarWrapper car, void* params)
{
	PriWrapper player = car.GetPRI();
	if (player.IsNull()) return;
	auto isLocalPlayer = player.IsLocalPlayerPRI();

	auto inGame = gameWrapper->IsInGame() || gameWrapper->IsInReplay() || gameWrapper->IsInOnlineGame();
	matchSettingsSema.acquire();
	bool inSm64Game = matchSettings.isInSm64Game;
	matchSettingsSema.release();
	if (!inGame || !inSm64Game)
	{
		return;
	}

	auto playerId = player.GetPlayerID();

	if (isHost || isLocalPlayer)
	{
		SM64MarioInstance* marioInstance = nullptr;

		ControllerInput* contInput = (ControllerInput*)params;
		if (isLocalPlayer)
		{
			marioInstance = &localMario;
			auto boostComponent = car.GetBoostComponent();
			if (!boostComponent.IsNull())
			{
				currentBoostAount = car.GetBoostComponent().GetCurrentBoostAmount();
			}

			if (menuStackCount > 0 &&
				(contInput->Jump || contInput->Handbrake || contInput->Throttle || contInput->Steer || contInput->Pitch))
			{
				// Reset the menu stack on car input just in case our stack count gets desynced
				menuStackCount = 0;
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
			if( marioState->posX == 0 && marioState->posY == 0 && marioState->posZ == 0 )
			{
				marioInstance->sema.release();
				return;
			}
			auto carPosition = Vector(marioState->posX, marioState->posZ, marioState->posY + CAR_OFFSET_Z);
			car.SetLocation(carPosition);
			car.SetVelocity(Vector(marioState->velX, marioState->velZ, marioState->velY));
			auto carRot = car.GetRotation();
			carRot.Yaw = marioYaw;
			carRot.Roll = carRotation.Roll;
			carRot.Pitch = carRotation.Pitch;
			car.SetRotation(carRot);
			contInput->Jump = 0;
			contInput->Handbrake = 0;
			contInput->Throttle = 0;
			contInput->Steer = 0;
			contInput->Pitch = 0;

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

void SM64::onNameplateTick(ServerWrapper caller, void* params)
{
	PrimitiveComponentWrapper nameplate = static_cast<PrimitiveComponentWrapper>(caller.memory_address);
	nameplate.SetbIgnoreOwnerHidden(true);
	auto translation = nameplate.GetTranslation();
}

void SM64::onCharacterSpawn(ServerWrapper server)
{
	HookEventWithCaller<ServerWrapper>(
		preGameTickCheck,
		[this](const ServerWrapper& caller, void* params, const std::string&) {
			onTick(caller);
		});
	matchSettingsSema.acquire();
	matchSettings.isPreGame = true;
	if (isHost && isActive)
	{
		matchSettings.isInSm64Game = true;
	}
	matchSettingsSema.release();
	sendSettingsIfHost(server);
}

void SM64::onCountdownEnd(ServerWrapper server)
{
	UnhookEvent(preGameTickCheck);
	matchSettingsSema.acquire();
	matchSettings.isPreGame = false;
	matchSettingsSema.release();
}

void SM64::onTick(ServerWrapper server)
{
	if (!isHost) return;
	matchSettingsSema.acquire();
	matchSettings.isInSm64Game = true;
	matchSettingsSema.release();

	int localMarioPlayerId = -1;
	for (CarWrapper car : server.GetCars())
	{
		PriWrapper player = car.GetPRI();
		if (player.IsNull()) continue;
		if (player.IsLocalPlayerPRI())
		{
			localMarioPlayerId = player.GetPlayerID();
			remoteMariosSema.acquire();
			tickMarioInstance(&localMario, car, this);
			remoteMariosSema.release();
		}

	}

}

void SM64::onOvertimeStart(ServerWrapper server)
{
	matchSettingsSema.acquire();
	bool inSm64Game = matchSettings.isInSm64Game;
	matchSettingsSema.release();
	if (inSm64Game)
	{
		OnGameLeft(false);
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
	else if (marioInstance->marioBodyState.marioState.posX == 0.0f &&
		marioInstance->marioBodyState.marioState.posY == 0.0f &&
		marioInstance->marioBodyState.marioState.posZ == 0.0f)
	{
		sm64_mario_delete(marioInstance->marioId);
		marioInstance->marioId = sm64_mario_create(x, z, y);
		if (marioInstance->marioId < 0) return;
	}

	auto camera = instance->gameWrapper->GetCamera();
	if (!camera.IsNull())
	{
		instance->cameraLoc = camera.GetLocation();
	}

	auto playerController = car.GetPlayerController();
	instance->matchSettingsSema.acquire();
	bool isPreGame = instance->matchSettings.isPreGame;
	instance->matchSettingsSema.release();
	if (!isPreGame && !playerController.IsNull())
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

			auto distance = self->utils.Distance(localMarioVector, remoteMarioVector);
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

	marioInstance->marioInputs.bljInput =  instance->matchSettings.bljSetup;

	instance->cameraLoc = camera.GetLocation();
	auto relMarioVec = Vector(marioInstance->marioState.posX - instance->cameraLoc.X,
		marioInstance->marioState.posZ - instance->cameraLoc.Y,
		marioInstance->marioState.posY - instance->cameraLoc.Z);
	auto quat = RotatorToQuat(camera.GetRotation()).conjugate();

	Vector adjustVec = Vector(0, 1, 0);
	Quat adjustQuat = RotatorToQuat(VectorToRotator(adjustVec));
	Vector viewPosition = RotateVectorWithQuat(RotateVectorWithQuat(relMarioVec, quat), adjustQuat);
	if (!self->gameWrapper->IsPaused())
	{
		sm64_mario_set_camera_to_object(viewPosition.X, viewPosition.Z, viewPosition.Y);
		sm64_mario_tick(marioInstance->marioId,
			&marioInstance->marioInputs,
			&marioInstance->marioState,
			&marioInstance->marioGeometry,
			&marioInstance->marioBodyState,
			true,
			true);
	}

	marioInstance->playerId = car.GetPRI().GetPlayerID();
	memcpy(self->netcodeOutBuf, &marioInstance->playerId, sizeof(int));
	memcpy(self->netcodeOutBuf + sizeof(int), &marioInstance->marioBodyState, sizeof(struct SM64MarioBodyState));
	Networking::SendBytes(self->netcodeOutBuf, sizeof(struct SM64MarioBodyState) + sizeof(int));
	marioInstance->sema.release();
}

void backgroundLoadData()
{
	self->utils.ParseObjFile(self->utils.GetBakkesmodFolderPath() + "data\\assets\\ROCKETBALL.obj", &self->ballVertices);
	self->backgroundLoadThreadFinished = true;
}

inline void renderMario(SM64MarioInstance* marioInstance, CameraWrapper camera)
{
	if (marioInstance == nullptr) return;

	if (self->menuStackCount > 0)
	{
		marioInstance->sema.acquire();
		if (marioInstance->mesh != nullptr)
		{
			marioInstance->mesh->RenderUpdateVertices(0, &camera);
		}
		marioInstance->sema.release();
		return;
	}

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

		if (marioInstance->colorIndex >= 0)
		{
			int trueIndex = 6 * marioInstance->colorIndex;
			marioInstance->mesh->SetCapColor(self->teamColors[trueIndex],
				self->teamColors[trueIndex + 1],
				self->teamColors[trueIndex + 2]);
			marioInstance->mesh->SetShirtColor(self->teamColors[trueIndex + 3],
				self->teamColors[trueIndex + 4],
				self->teamColors[trueIndex + 5]);
		}

		marioInstance->mesh->SetShowAltTexture(marioInstance->isStem);
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
				stemTexture,
				4 * SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT,
				SM64_TEXTURE_WIDTH,
				SM64_TEXTURE_HEIGHT));
		}
		marioMeshPoolSema.release();

		meshesInitialized = true;
	}

	sm64_audio_tick();

	auto inGame = gameWrapper->IsInGame() || gameWrapper->IsInReplay() || gameWrapper->IsInOnlineGame();
	matchSettingsSema.acquire();
	bool inSm64Game = matchSettings.isInSm64Game;
	matchSettingsSema.release();
	if (!inGame && inSm64Game)
	{
		OnGameLeft(true);
	}
	if (!inGame || !inSm64Game)
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

	bool needsSettingSync = false;

	// Render local mario, and mark which marios no longer exist
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

		auto playerId = player.GetPlayerID();
		if (remoteMarios.count(playerId) > 0)
		{
			auto remoteMario = remoteMarios[playerId];
			remoteMario->CarActive = true;
			remoteMario->teamIndex = teamIndex;
			if (remoteMario->mesh != nullptr)
			{
				//remoteMario->mesh->ShowNameplate(L"", car.GetLocation());
			}
			if (isHost && remoteMario->colorIndex < 0)
			{
				remoteMario->colorIndex = getColorIndexFromPool(teamIndex);
				needsSettingSync = true;
			}
		}

		if (playerName != localPlayerName)
		{
			continue;
		}

		localMario.CarActive = true;
		localMario.teamIndex = teamIndex;

		SM64MarioInstance* marioInstance = &localMario;


		carLocation = car.GetLocation();

		if (marioInstance->mesh == nullptr)
		{
			marioInstance->mesh = getMeshFromPool();
		}
		else
		{
			//marioInstance->mesh->ShowNameplate(L"", car.GetLocation());
		}

		if(!isHost)
		{
			tickMarioInstance(marioInstance, car, this);
		}
		if (isHost && localMario.colorIndex < 0)
		{
			localMario.colorIndex = getColorIndexFromPool(teamIndex);
			needsSettingSync = true;
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

		if (isHost && localMario.colorIndex >= 0)
		{
			addColorIndexToPool(localMario.colorIndex);
			localMario.colorIndex = -1;
			needsSettingSync = true;
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

			if (isHost && marioInstance->colorIndex >= 0)
			{
				addColorIndexToPool(marioInstance->colorIndex);
				marioInstance->colorIndex = -1;
				needsSettingSync = true;
			}
		}

		marioInstance->sema.release();

		renderMario(marioInstance, camera);
	}
	remoteMariosSema.release();

	if (needsSettingSync)
	{
		sendSettingsIfHost(server);
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

int SM64::getColorIndexFromPool(int teamIndex)
{
	std::vector<int>* colorPool = &blueTeamColorIndexPool;
	if (teamIndex == 1)
	{
		colorPool = &redTeamColorIndexPool;
	}
	int colorIndex = 0;
	if (colorPool->size() > 0)
	{
		colorIndex = (*colorPool)[0];
		colorPool->erase(colorPool->begin());
	}
	return colorIndex;
}

void SM64::addColorIndexToPool(int colorIndex)
{
	std::vector<int>* colorPool = &blueTeamColorIndexPool;
	if (colorIndex < TEAM_COLOR_POOL_SIZE)
	{
		colorPool = &redTeamColorIndexPool;
	}
	colorPool->insert(colorPool->begin(), colorIndex);
}

void audioCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	for (size_t i = 0; i < frameCount; i += DEFAULT_LEN_2CH)
	{
		const size_t count = min(i + DEFAULT_LEN_2CH, frameCount) - i;
		sm64_create_next_audio_buffer((int16_t*)pOutput + i, count);
	}

	(void)pInput;
}

void initAudio()
{
	std::string bakkesmodFolderPath = self->utils.GetBakkesmodFolderPath();
	std::string romPath = bakkesmodFolderPath + "data\\assets\\baserom.us.z64";
	std::string extractAssetsPath = bakkesmodFolderPath + "data\\assets\\genfiles.exe";
	std::string extractAssetsPathWithArgs = extractAssetsPath + " " + romPath;
	if (!self->utils.FileExists(extractAssetsPath))
	{
		return;
	}

	std::string tempDir = std::filesystem::temp_directory_path().string();
	std::string bankSetsPath = tempDir + "bank_sets";
	std::string sequencesPath = tempDir + "sequences.bin";
	std::string soundDataCtlPath = tempDir + "sound_data.ctl";
	std::string soundDataTblPath = tempDir + "sound_data.tbl";

	// additional information
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	// set the size of the structures
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	CreateProcessA(NULL, (LPSTR)extractAssetsPathWithArgs.c_str(), NULL, NULL,
		FALSE, CREATE_NO_WINDOW, NULL, tempDir.c_str(), &si, &pi);

	WaitForSingleObject(pi.hProcess, 2000);

	if (self->utils.FileExists(bankSetsPath))
	{
		size_t bankSetsSize, sequencesSize, soundDataCtlSize, soundDataTblSize;
		auto bankSets = self->utils.ReadAllBytes(bankSetsPath, bankSetsSize);
		auto sequencesBin = self->utils.ReadAllBytes(sequencesPath, sequencesSize);
		auto soundDataCtl = self->utils.ReadAllBytes(soundDataCtlPath, soundDataCtlSize);
		auto soundDataTbl = self->utils.ReadAllBytes(soundDataTblPath, soundDataTblSize);

		sm64_load_sound_data(bankSets.get(),
			sequencesBin.get(),
			soundDataCtl.get(),
			soundDataTbl.get(),
			bankSetsSize,
			sequencesSize,
			soundDataCtlSize,
			soundDataTblSize);

		std::remove(bankSetsPath.c_str());
		std::remove(sequencesPath.c_str());
		std::remove(soundDataCtlPath.c_str());
		std::remove(soundDataTblPath.c_str());


		ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
		deviceConfig.sampleRate = 32000;
		deviceConfig.periodSizeInFrames = 528;
		deviceConfig.periods = 2;
		deviceConfig.dataCallback = audioCallback;
		deviceConfig.playback.format = ma_format_s16;
		deviceConfig.playback.channels = 2;

		ma_device_init(nullptr, &deviceConfig, &device);
		ma_device_start(&device);

		int initialVolume = MarioConfig::getInstance().GetVolume();
		device.masterVolumeFactor = initialVolume / 100.0f;

		audioInitialized = true;
	}

}
