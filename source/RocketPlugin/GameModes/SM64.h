#pragma once

#pragma comment(lib, "sm64.lib")

#include <fstream>
#include <semaphore>
#include <iostream>
#include <tchar.h>
#include <shlwapi.h>
#include <stdlib.h>
#pragma comment(lib,"shlwapi.lib")
#include "shlobj.h"

#include "../Graphics/Renderer.h"
#include "../Graphics/Mesh.h"
#include "../Modules/Utils.h"
#include "../Modules/MarioAudio.h"
#include "imgui/imgui.h"
#include "imgui/imgui_additions.h"
#include "GameModes/RocketGameMode.h"
#include "../../External/NetcodeManager/NetcodeManager.h"

extern "C" {
    #include "libsm64.h"
}

#include "../Graphics/level.h"


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
    Mesh* mesh = nullptr;
    std::counting_semaphore<1> sema{ 1 };
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
    void StopRenderMario(std::string eventName);
    void OnCarSpawned(std::string eventName);

    void InitSM64();
    void DestroySM64();
    void OnMessageReceived(const std::string& message, PriWrapper sender);
    void OnRender(CanvasWrapper canvas);

    std::string bytesToHex(unsigned char* data, unsigned int len);

private:
    float distance(Vector v1, Vector v2);
    void onTick(ServerWrapper server);
    void onVehicleTick(CarWrapper car);
    std::vector<char> hexToBytes(const std::string& hex);
    uint8_t* utilsReadFileAlloc(std::string path, size_t* fileLength);

public:
    MarioAudio* marioAudio = nullptr;
    std::shared_ptr<GameWrapper> gameWrapper;
    Vector cameraLoc = Vector(0, 0, 0);
    ControllerInput playerInputs;
    std::shared_ptr<NetcodeManager> Netcode;
private:
    /* SM64 Members */
    uint8_t* texture;
    std::map<std::string, SM64MarioInstance*> remoteMarios;
    SM64MarioInstance localMario;
    vec3 cameraPos;
    float cameraRot;
    Vector carLocation;
    bool locationInit;
    Rotator carRotation;
    Renderer* renderer = nullptr;
    Mesh* marioMesh = nullptr;
    Mesh* ballMesh = nullptr;
    bool sm64Initialized = false;
    bool meshesInitialized = false;
    struct SM64MarioBodyState marioBodyStateIn;
    bool renderLocalMario = false;
    bool renderRemoteMario = false;
    std::shared_ptr<CVarManagerWrapper> cvarManager;
    Utils utils;
    bool isHost = false;

};