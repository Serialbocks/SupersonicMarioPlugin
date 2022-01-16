#pragma once

#pragma comment(lib, "sm64.lib")

#include <fstream>
#include "../Modules/Renderer.h"

#include "imgui/imgui.h"
#include "imgui/imgui_additions.h"
#include "../../External/RenderingTools/Objects/Triangle.h"
#include "../../External/RenderingTools/Objects/Frustum.h"

extern "C" {
    #include "libsm64.h"
}

#include "../Modules/level.h"

#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_CXX17_C_HEADER_DEPRECATION_WARNING

class SM64Plugin final : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginWindow
{
public:
    virtual void onLoad();
    virtual void onUnload();
    void RenderMario(CanvasWrapper canvas);
    void OnFreeplayLoad(std::string eventName);
    void OnFreeplayDestroy(std::string eventName);

private:
    void DebugRenderLevel(CanvasWrapper canvas, RT::Frustum frust);
    float Distance(Vector v1, Vector v2);

public:
    /* Window settings */
    virtual void Render();
    virtual std::string GetMenuName();
    virtual std::string GetMenuTitle();
    virtual void SetImGuiContext(uintptr_t ctx);
    virtual bool ShouldBlockInput();
    virtual bool IsActiveOverlay();
    virtual void OnOpen();
    virtual void OnClose();
private:
    bool isWindowOpen = false;
    bool isMinimized = false;
    std::string menuTitle = "SM64";
    std::string statusText = "test status";

private:
    /* SM64 Members */
    uint8_t* texture;
    int32_t marioId;
    struct SM64MarioInputs marioInputs;
    struct SM64MarioState marioState;
    struct SM64MarioGeometryBuffers marioGeometry;
    vec3 cameraPos;
    float cameraRot;
    Vector v;
    bool locationInit;
    float marioOffsetZ = 0;
    float carOffsetZ = 20.0f;
    int depthFactor = 20000;
    bool drawOutline;
    float lineThickness;
    std::thread* marioThread;
    ControllerInput playerInputs;
    Vector cameraLoc;
    Rotator carRotation;
    Renderer* renderer = nullptr;
    Renderer::MeshVertex* projectedVertices = nullptr;

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