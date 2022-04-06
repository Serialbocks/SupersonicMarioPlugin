#pragma once

#include <d3d11.h>
#include <Windows.h>
#include "../../External/kiero/kiero.h"
#include "shaders.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <fstream>
#include <bakkesmod/wrappers/wrapperstructs.h>
#include "Mesh.h"
#include "Lighting.h"
#include "GraphicsTypes.h"
#include "Modules/Utils.h"

#pragma comment(lib, "d3dcompiler.lib")

#define safe_release(p) if (p) { p->Release(); p = nullptr; } 

class Mesh;

class Renderer
{
public:
	Renderer();
	~Renderer();
	Mesh* CreateMesh(size_t maxTriangles,
		uint8_t* inTexture = nullptr,
		uint8_t* inAltTexture = nullptr,
		size_t inTexSize = 0,
		uint16_t inTexWidth = 0,
		uint16_t inTexHeight = 0);
	Mesh* CreateMesh(std::vector<Vertex>* inVertices,
		uint8_t* inTexture = nullptr,
		size_t inTexSize = 0,
		uint16_t inTexWidth = 0,
		uint16_t inTexHeight = 0);
	bool Init(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags);
	void OnPresent(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags);
	bool Initialized = false;
	PS_ConstantBufferData PixelConstBufferData;
	Lighting Lighting;
	HWND Window = nullptr;

private:
	Utils utils;

private:
	void Render();
	void CreatePipeline();
	Microsoft::WRL::ComPtr<ID3DBlob> LoadShader(const char* shaderData, std::string targetShaderVersion, std::string shaderEntry);
	void InitMeshBuffers();
	void DrawRenderedMesh();
	
	bool drawMeshes = false;
	bool pipelineInitialized = false;
	bool firstInit = true;
	int windowWidth, windowHeight;
	std::vector<Mesh*> meshes;

	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Device> device = nullptr;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mainRenderTargetView = nullptr;
	D3D11_VIEWPORT viewport;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader = nullptr;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShaderTextures = nullptr;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShaderTexturesTransparent = nullptr;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader = nullptr;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout = nullptr;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState = nullptr;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView = nullptr;
	Microsoft::WRL::ComPtr<ID3D11BlendState> blendState = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> pixelConstantBuffer = nullptr;
};