#pragma once

#include "Renderer.h"
#include "../Modules/Utils.h"
#include <WICTextureLoader.h>

class Renderer;

class Mesh
{
public:
	typedef struct MeshVertex_t
	{
		Vector position;
		float r, g, b, a;
		float u, v;
	} MeshVertex;
	Mesh(Microsoft::WRL::ComPtr<ID3D11Device> deviceIn,
		int inWindowWidth,
		int inWindowHeight,
		size_t maxTriangles,
		uint8_t* inTexture = nullptr,
		size_t inTexSize = 0,
		uint16_t inTexWidth = 0,
		uint16_t inTexHeight = 0);
	Mesh(Microsoft::WRL::ComPtr<ID3D11Device> deviceIn,
		int inWindowWidth,
		int inWindowHeight,
		std::string objFileName,
		uint8_t* inTexture,
		size_t inTexSize,
		uint16_t inTexWidth,
		uint16_t inTexHeight);
	void Render(CameraWrapper *camera);
	void RenderUpdateVertices(size_t numTrianglesUsed, CameraWrapper *camera);
	void SetTranslation(float x, float y, float z);
	void SetScale(float x, float y, float z);
	void SetRotation(float roll, float pitch, float yaw);
	void SetRotationQuat(float x, float y, float z, float w);

private:
	void init(Microsoft::WRL::ComPtr<ID3D11Device> deviceIn,
		int inWindowWidth,
		int inWindowHeight,
		size_t maxTriangles,
		uint8_t* inTexture = nullptr,
		size_t inTexSize = 0,
		uint16_t inTexWidth = 0,
		uint16_t inTexHeight = 0);
	void parseObjFile(std::string path);
	std::vector<std::string> splitStr(std::string str, char delimiter);
	Utils utils;
	float rotRoll, rotPitch, rotYaw = 0.0f;
	float quatX, quatY, quatZ, quatW = 0.0f;

public:
	struct Vertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 texCoord;
		DirectX::XMFLOAT3 normal;
	};
	typedef struct VS_ConstantBufferData_t
	{
		DirectX::XMMATRIX wvp = DirectX::XMMatrixIdentity();
		DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
	} VS_ConstantBufferData;
	VS_ConstantBufferData VertexConstBufferData;

	typedef struct PS_ConstantBufferData_t
	{
		DirectX::XMFLOAT3 ambientLightColor;
		float ambientLightStrength = 0.7f;

		DirectX::XMFLOAT3 dynamicLightColor;
		float dynamicLightStrength = 1.0f;
		DirectX::XMFLOAT3 dynamicLightPosition;
	} PS_ConstantBufferData;
	PS_ConstantBufferData PixelConstBufferData;

	size_t MaxTriangles = 0;
	size_t NumTrianglesUsed = 0;
	bool UpdateVertices = false;
	std::vector<Vertex> Vertices;
	std::vector<unsigned int> Indices;
	size_t NumIndices = 0;
	bool IsTransparent = false;
	Microsoft::WRL::ComPtr<ID3D11Buffer> VertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> IndexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> VertexConstantBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> PixelConstantBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> TextureResourceView = nullptr;

private:
	uint8_t* texData = nullptr;
	size_t texSize;
	uint16_t texWidth, texHeight;
	int windowWidth, windowHeight;
	const DirectX::XMVECTOR DEFAULT_UP_VECTOR = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	Vector translationVector = Vector(0.0f, 0.0f, 0.0f);
	Vector scaleVector = Vector(1.0f, 1.0f, 1.0f);
	Vector rotationVector = Vector(0.0f, 0.0f, 0.0f);

	Microsoft::WRL::ComPtr<ID3D11Device> device = nullptr;

};