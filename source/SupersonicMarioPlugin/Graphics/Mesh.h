#pragma once

#include "Renderer.h"
#include "../Modules/Utils.h"
#include <WICTextureLoader.h>
#include "GraphicsTypes.h"

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

	VS_ConstantBufferData VertexConstBufferData;
	Mesh(Microsoft::WRL::ComPtr<ID3D11Device> deviceIn,
		int inWindowWidth,
		int inWindowHeight,
		size_t maxTriangles,
		uint8_t* inTexture = nullptr,
		uint8_t* inAltTexture = nullptr,
		size_t inTexSize = 0,
		uint16_t inTexWidth = 0,
		uint16_t inTexHeight = 0);
	Mesh(Microsoft::WRL::ComPtr<ID3D11Device> deviceIn,
		int inWindowWidth,
		int inWindowHeight,
		std::vector<Vertex>* inVertices,
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
	void SetCapColor(float r, float g, float b);
	void SetShirtColor(float r, float g, float b);
	void SetShowAltTexture(bool val);

private:
	void init(Microsoft::WRL::ComPtr<ID3D11Device> deviceIn,
		int inWindowWidth,
		int inWindowHeight,
		size_t maxTriangles,
		uint8_t* inTexture = nullptr,
		uint8_t* inAltTexture = nullptr,
		size_t inTexSize = 0,
		uint16_t inTexWidth = 0,
		uint16_t inTexHeight = 0);
	float rotRoll, rotPitch, rotYaw = 0.0f;
	float quatX, quatY, quatZ, quatW = 0.0f;

public:
	size_t MaxTriangles = 0;
	size_t NumTrianglesUsed = 0;
	bool UpdateVertices = false;
	std::vector<Vertex> Vertices;
	std::vector<unsigned int> Indices;
	size_t NumIndices = 0;
	bool IsTransparent = false;
	bool ShowAltTexture = false;
	Microsoft::WRL::ComPtr<ID3D11Buffer> VertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> IndexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> VertexConstantBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> TextureResourceView = nullptr;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> AltTextureResourceView = nullptr;
	float CapColorR, CapColorG, CapColorB;
	float ShirtColorR, ShirtColorG, ShirtColorB;
	bool render = false;

private:
	uint8_t* texData = nullptr;
	uint8_t* altTexData = nullptr;
	size_t texSize;
	uint16_t texWidth, texHeight;
	int windowWidth, windowHeight;
	const DirectX::XMVECTOR DEFAULT_UP_VECTOR = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	Vector translationVector = Vector(0.0f, 0.0f, 0.0f);
	Vector scaleVector = Vector(1.0f, 1.0f, 1.0f);
	Vector rotationVector = Vector(0.0f, 0.0f, 0.0f);
	Microsoft::WRL::ComPtr<ID3D11Device> device = nullptr;

};