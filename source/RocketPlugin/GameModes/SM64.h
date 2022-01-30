#pragma once

#pragma comment(lib, "sm64.lib")

#include <fstream>
#include <semaphore>
#include <iostream>
#include <tchar.h>
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")
#include "shlobj.h"

#include "../Modules/Renderer.h"
#include "imgui/imgui.h"
#include "imgui/imgui_additions.h"
#include "../../External/RenderingTools/Objects/Frustum.h"
#include "../../External/NetcodeManager/NetcodeManager.h"
#include "GameModes/RocketGameMode.h"

extern "C" {
    #include "libsm64.h"
}

#include "../Modules/level.h"

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
    void RenderMario(CanvasWrapper canvas);

private:
    float distance(Vector v1, Vector v2);
    void onTick(ServerWrapper server);
    std::string bytesToHex(unsigned char* data, unsigned int len);
    std::vector<char> hexToBytes(const std::string& hex);
    std::string getBakkesmodFolderPath();
    uint8_t* utilsReadFileAlloc(std::string path, size_t* fileLength);

private:
    /* SM64 Members */
    uint8_t* texture;
    int32_t marioId;
    int32_t remoteMarioId;
    struct SM64MarioInputs marioInputs;
    struct SM64MarioState marioState;
    struct SM64MarioGeometryBuffers marioGeometry;
    vec3 cameraPos;
    float cameraRot;
    Vector carLocation;
    bool locationInit;
    ControllerInput playerInputs;
    Vector cameraLoc = Vector(0, 0, 0);
    Rotator carRotation;
    Renderer* renderer = nullptr;
    Renderer::MeshVertex* projectedVertices = nullptr;
    bool sm64Initialized = false;
    struct SM64MarioBodyState marioBodyState;
    struct SM64MarioBodyState marioBodyStateIn;
    bool renderLocalMario = false;
    bool renderRemoteMario = false;
    std::shared_ptr<NetcodeManager> Netcode;
    std::shared_ptr<GameWrapper> gameWrapper;
    std::shared_ptr<CVarManagerWrapper> cvarManager;

};