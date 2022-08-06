#pragma once

#pragma comment(lib, "sm64.lib")
#pragma comment(lib,"shlwapi.lib")

#include <fstream>
#include <semaphore>
#include <iostream>
#include <sstream>
#include <tchar.h>
#include <shlwapi.h>
#include <stdlib.h>
#include <math.h>
#include <libloaderapi.h>

#include "shlobj.h"

#include "../Graphics/Renderer.h"
#include "../Graphics/GraphicsTypes.h"
#include "../Graphics/Model.h"
#include "../Graphics/surface_terrains.h"
#include "../Modules/Utils.h"
#include "../Modules/MarioAudio.h"
#include "../Modules/MarioConfig.h"
#include "../Modules/Update.h"
#include "imgui/imgui.h"
#include "imgui/imgui_additions.h"
#include "imgui/imgui_internal.h"
#include "GameModes/RocketGameMode.h"
#include "../../External/BakkesModSDK/include/bakkesmod/wrappers/PluginManagerWrapper.h"
#include "Networking/Networking.h"
#include "xxHash/xxhash.h"

extern "C" {
    #include "libsm64.h"
}

#include "../Graphics/level.h"

#define SM64_NETCODE_BUF_LEN 1024
#define MARIO_MESH_POOL_SIZE 10
#define TEAM_COLOR_POOL_SIZE 4
#define MAX_NUM_PLAYERS 8
#define Z_MAP_OFFSET 1024

#ifndef minV
#define minV(a, b) ((a) <= (b) ? (a) : (b))
#endif

#ifndef maxV
#define maxV(a, b) ((a) > (b) ? (a) : (b))
#endif

struct MatchSettings
{
    SM64MarioBljInput bljSetup;
    bool isPreGame = false;
    bool isInSm64Game = false;

    int playerCount = 0;
    int playerIds[MAX_NUM_PLAYERS] = { 0 };
    int playerColorIndices[MAX_NUM_PLAYERS] = { 0 };
    bool playerIsCarFlags[MAX_NUM_PLAYERS] = { 0 };

    bool joinGame = false;
};

class SM64MarioInstance
{
public:
    SM64MarioInstance();
    ~SM64MarioInstance();

public:
    int32_t marioId = -2;
    struct SM64MarioInputs marioInputs { 0 };
    struct SM64MarioState marioState { 0 };
    struct SM64MarioGeometryBuffers marioGeometry { 0 };
    struct SM64MarioBodyState marioBodyState { 0 };
    bool MarioActive = true;
    Model* model = nullptr;
    std::counting_semaphore<1> sema{ 1 };
    int slidingHandle = -1;
    int yahooHandle = -1;
    int colorIndex = -1;
    int playerId = -1;
    int teamIndex = -1;
    unsigned long tickCount = 0;
    unsigned long lastBallInteraction = 0;
    bool isCar = false;
};

class SM64 final : public RocketGameMode
{
public:
    SM64(std::shared_ptr<GameWrapper> gw,
        std::shared_ptr<CVarManagerWrapper> cm,
        BakkesMod::Plugin::PluginInfo exports);
    ~SM64();

    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;
    void RenderPreferences();

    void InitSM64();
    void DestroySM64();
    void OnRender(CanvasWrapper canvas);

    void OnGameLeft(bool deleteMario);

    void SendSettingsToClients();

    void MatchSettingsMessageReceived(char* buf, int len);
    void MarioMessageReceived(char* buf, int len);
    void SendJoinCommandToClients();

    void LoadStaticSurfaces(Model* model = nullptr);

private:
    void onCharacterSpawn(ServerWrapper server);
    void onCountdownEnd(ServerWrapper server);
    void onOvertimeStart(ServerWrapper server);
    void onTick(ServerWrapper server);
    void onSetVehicleInput(CarWrapper car, void* params);
    void onNameplateTick(ServerWrapper caller, void* params);
    void sendSettingsIfHost(ServerWrapper server);
    void moveCarToMario(std::string eventName);
    void onGoalScored(std::string eventName);
    void menuPushed(ServerWrapper server);
    void menuPopped(ServerWrapper server);
    uint8_t* utilsReadFileAlloc(std::string path, size_t* fileLength);
    Model* getModelFromPool();
    void addModelToPool(Model*);
    int getColorIndexFromPool(int teamIndex);
    void addColorIndexToPool(int colorIndex);

public:
    //Model* testMapModel;
    //std::vector<Vertex> testMapModelVertices;
    SM64MarioInstance localMario;
    std::shared_ptr<GameWrapper> gameWrapper;
    Vector cameraLoc = Vector(0, 0, 0);
    ControllerInput playerInputs;
    Rotator carRotation;
    char netcodeOutBuf[SM64_NETCODE_BUF_LEN];
    std::vector<Model*> marioModelPool;
    std::counting_semaphore<1> marioModelPoolSema{ 1 };
    float currentBoostAount = 0.33f;
    std::map<int, SM64MarioInstance*> remoteMarios;
    std::counting_semaphore<1> remoteMariosSema{ 1 };
    Vector carLocation;
    MatchSettings matchSettings;
    std::counting_semaphore<1> matchSettingsSema{ 1 };
    std::vector<int> redTeamColorIndexPool = { 0, 1, 2, 3 };
    std::vector<int> blueTeamColorIndexPool = { 4, 5, 6, 7 };
    std::vector<float> teamColors = {
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, // Red team
        1.0f, 0.389f, 0.0f, 0.0f, 0.517f, 0.803f,
        1.0f, 0.722f, 0.0f, 0.464f, 0.0f, 0.472f,
        1.0f, 0.15f, 0.0f, 0.508f, 0.508f, 0.508f,
        0.0f, 0.719f, 0.156f, 0.0f, 0.0f, 1.0f, // Blue team
        0.586f, 0.0f, 0.869f, 0.0f, 0.0f, 0.606f,
        0.0f, 0.122f, 0.778f, 0.031f, 0.031f, 0.031f,
        0.983f, 0.0f, 0.717f, 0.0f, 0.0f, 0.0f
    };
    int menuStackCount = 0;
    bool Sm64Initialized = false;
    uint32_t interpolationInterval = 1;

    Model* ballModel = nullptr;
    Model* octaneModel = nullptr;
    Model* dominusModel = nullptr;
    Model* fennecModel = nullptr;
    Model* mapModel = nullptr;

private:
    /* SM64 Members */
    uint8_t* texture = nullptr;
    vec3 cameraPos;
    float cameraRot;
    bool locationInit;
    Model* marioModel = nullptr;
    bool modelsInitialized = false;
    struct SM64MarioBodyState marioBodyStateIn;
    std::shared_ptr<CVarManagerWrapper> cvarManager;
    bool isHost = false;
    float groundPoundPinchVel;
    float attackBallRadius;
    float kickBallVelHoriz;
    float kickBallVelVert;
    float punchBallVelHoriz;
    float punchBallVelVert;
    float attackBoostDamage;
    float diveBallVelHoriz;
    float diveBallVelVert;
    float flyBallVel;
    int interpolationFactor = 1;

protected:
    const std::string vehicleInputCheck = "Function TAGame.Car_TA.SetVehicleInput";
    const std::string initialCharacterSpawnCheck = "Function GameEvent_Soccar_TA.WaitingForPlayers.BeginState";
    const std::string CharacterSpawnCheck = "Function GameEvent_TA.Countdown.BeginState"; //.OnPlayerRestarted might also work
    const std::string preGameTickCheck = "Function TAGame.GameEvent_Soccar_TA.ShowSeasonIntroScene";
    const std::string endPreGameTickCheck = "Function GameEvent_TA.Countdown.EndState";
    const std::string clientEndPreGameTickCheck = "Function GameEvent_Soccar_TA.Countdown.EndState";
    const std::string gameTickCheck = "Function GameEvent_Soccar_TA.Active.Tick";
    const std::string overtimeGameCheck = "Function TAGame.GameEvent_Soccar_TA.StartOvertime";
    const std::string clientOvertimeGameCheck = "Function TAGame.GameEvent_Soccar_TA.OnOvertimeUpdated";

    const std::string playerLeaveOrJoinCheck = "Function TAGame.ListenServer_TA.GetCustomMatchSettings";
    const std::string playerJoinedTeamCheck = "Function TAGame.GameMetrics_TA.JoinTeam";
    const std::string playerLeftTeamCheck = "Function TAGame.GameMetrics_TA.LeaveTeam";
    const std::string menuPushCheck = "Function TAGame.GFxData_MenuStack_TA.PushMenu";
    const std::string menuPopCheck = "Function TAGame.GFxData_MenuStack_TA.PopMenu";
    const std::string nameplateTickCheck = "Function TAGame.NameplateMeshComponent_TA.TickNameplate";
};