#pragma once

#pragma comment(lib, "sm64.lib")

#include <fstream>
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
    SM64() { typeIdx = std::make_unique<std::type_index>(typeid(*this)); }
    SM64(std::shared_ptr<GameWrapper> gw,
        std::shared_ptr<CVarManagerWrapper> cm,
        BakkesMod::Plugin::PluginInfo exports);

    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;

    virtual void onLoad();
    virtual void onUnload();
    void InitSM64();
    void DestroySM64();
    void OnMessageReceived(const std::string& message, PriWrapper sender);

private:
    float distance(Vector v1, Vector v2);
    void onTick(ServerWrapper server);

private:
    /* SM64 Members */
    uint8_t* texture;
    int32_t marioId;
    struct SM64MarioInputs marioInputs;
    struct SM64MarioState marioState;
    struct SM64MarioGeometryBuffers marioGeometry;
    vec3 cameraPos;
    float cameraRot;
    Vector carLocation;
    bool locationInit;
    float marioOffsetZ = 0;
    float carOffsetZ = 20.0f;
    int depthFactor = 20000;
    ControllerInput playerInputs;
    Vector cameraLoc;
    Rotator carRotation;
    Renderer* renderer = nullptr;
    Renderer::MeshVertex* projectedVertices = nullptr;
    bool sm64Initialized = false;
    struct SM64MarioBodyState marioBodyState;
    std::shared_ptr<NetcodeManager> Netcode;
    std::shared_ptr<GameWrapper> gameWrapper;
    std::shared_ptr<CVarManagerWrapper> cvarManager;

    std::string GetCurrentDirectory()
    {
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::string::size_type pos = std::string(buffer).find_last_of("\\/");

        return std::string(buffer).substr(0, pos);
    }

    uint8_t* utilsReadFileAlloc(std::string path, size_t* fileLength)
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

};