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

#pragma comment(lib, "d3dcompiler.lib")

#define safe_release(p) if (p) { p->Release(); p = nullptr; } 

class Renderer
{
public:
	typedef struct MeshVertex_t
	{
		Vector position;
		float r, g, b, a;
		float u, v;
	} MeshVertex;

	Renderer(uint16_t maxTriangles, uint8_t* inTexture, size_t texSize, uint16_t texWidth, uint16_t texHeight);
	~Renderer();
	bool Init(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags);
	void RenderMesh(
		MeshVertex* vertices,
		uint16_t numTrianglesUsed);
	void OnPresent(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags);

private:
	void Render();
	void CreatePipeline();
	Microsoft::WRL::ComPtr<ID3DBlob> LoadShader(const char* shaderData, std::string targetShaderVersion, std::string shaderEntry);
	void InitMeshBuffers();
	void DrawRenderedMesh();

	struct Vertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 texCoord;
	};

	struct ConstantBufferData
	{
		DirectX::XMMATRIX wvp = DirectX::XMMatrixIdentity();
	}
	constantBufferData;

	uint16_t MaxTriangles;
	uint16_t NumTrianglesUsed = 0;
	bool drawExamples = false;
	bool examplesLoaded = false;
	bool initialized = false;
	bool firstInit = true;
	int windowWidth, windowHeight;
	Vertex* vertices = nullptr;
	unsigned int* indices;
	UINT numIndices = 0;
	bool renderFrame = false;
	uint8_t* texData = nullptr;
	size_t texSize;
	uint16_t texWidth, texHeight;

	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Device> device = nullptr;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mainRenderTargetView = nullptr;
	D3D11_VIEWPORT viewport;

	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader = nullptr;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShaderTextures = nullptr;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader = nullptr;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout = nullptr;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState = nullptr;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureResourceView = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView = nullptr;
};