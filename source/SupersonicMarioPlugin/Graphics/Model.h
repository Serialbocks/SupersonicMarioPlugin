#pragma once
#include "GraphicsTypes.h"
#include "Mesh.h"
#include "Modules/Utils.h"
#include "Renderer.h"

#include <semaphore>

class Mesh;

class Model
{
public:
	Model(std::string path);
	Model(size_t inMaxTriangles,
		uint8_t* inTexture,
		uint8_t* inAltTexture,
		size_t inTexSize,
		uint16_t inTexWidth,
		uint16_t inTexHeight);
	bool NeedsInitialized();
	bool ShouldRender();
	void InitMeshes(Microsoft::WRL::ComPtr<ID3D11Device> device, int windowWidth, int windowHeight);
	void ParseObjFile(std::vector<Vertex>* vertices, std::vector<UINT>* indices);

	void Render(CameraWrapper* camera);
	void RenderUpdateVertices(size_t numTrianglesUsed, CameraWrapper* camera);
	void SetTranslation(float x, float y, float z);
	void SetScale(float x, float y, float z);
	void SetRotation(float roll, float pitch, float yaw);
	void SetRotationQuat(float x, float y, float z, float w);
	void SetCapColor(float r, float g, float b);
	void SetShirtColor(float r, float g, float b);
	void SetShowAltTexture(bool val);

	std::vector<Vertex>* GetVertices();
private:

public:
	std::vector<Mesh*> Meshes;
	std::vector<std::vector<Vertex>> modelVerticesArr;
	std::vector<std::vector<UINT>> modelIndicesArr;
	std::counting_semaphore<1> sema{ 1 };
	bool backgroundDataLoaded = false;
private:
	bool meshesInitialized = false;
	std::string modelPath;

	// Single mesh init vals
	size_t maxTriangles = 0;
	uint8_t* texture = nullptr;
	uint8_t* altTexture = nullptr;
	size_t texSize = 0;
	uint16_t texWidth = 0;
	uint16_t texHeight = 0;

};