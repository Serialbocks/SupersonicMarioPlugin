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
#include "Lighting.h"
#include "GraphicsTypes.h"
#include "Modules/Utils.h"
#include "Model.h"

#pragma comment(lib, "d3dcompiler.lib")

#define safe_release(p) if (p) { p->Release(); p = nullptr; } 

class Model;

class Renderer
{
public:
	static Renderer& getInstance()
	{
		static Renderer instance;
		return instance;
	}
	void AddModel(Model* model);
	bool Init(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags);
	void OnPresent(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags);
	bool Initialized = false;
	PS_ConstantBufferData PixelConstBufferData;
	Lighting Lighting;
	HWND Window = nullptr;

private:

private:
	Renderer();
	~Renderer();
	void Render();
	void CreatePipeline();
	Microsoft::WRL::ComPtr<ID3DBlob> LoadShader(const char* shaderData, std::string targetShaderVersion, std::string shaderEntry);
	void InitBuffers();
	void DrawModels();
	
	bool drawModels = false;
	bool pipelineInitialized = false;
	bool firstInit = true;
	int windowWidth, windowHeight;
	std::vector<Model*> models;

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