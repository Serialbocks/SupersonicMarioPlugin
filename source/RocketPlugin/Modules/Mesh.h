#pragma once

#include "Renderer.h"

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
	void Render(
		std::vector<MeshVertex> vertices,
		size_t numTrianglesUsed);

public:
	struct Vertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 texCoord;
	};
	size_t MaxTriangles = 0;
	size_t NumTrianglesUsed = 0;
	bool RenderFrame = false;
	std::vector<Vertex> Vertices;
	std::vector<unsigned int> Indices;
	size_t NumIndices = 0;
	Microsoft::WRL::ComPtr<ID3D11Buffer> VertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> IndexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> TextureResourceView = nullptr;

private:
	uint8_t* texData = nullptr;
	size_t texSize;
	uint16_t texWidth, texHeight;
	int windowWidth, windowHeight;

	Microsoft::WRL::ComPtr<ID3D11Device> device = nullptr;

};