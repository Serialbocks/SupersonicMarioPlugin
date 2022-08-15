#pragma once
#include "GraphicsTypes.h"
#include "Mesh.h"
#include "Modules/Utils.h"
#include "Renderer.h"

#pragma comment(lib, "assimp-vc142-mt.lib")
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <semaphore>

class Mesh;

class Model
{
public:
	typedef struct Frame_t
	{
		float rotRoll, rotPitch, rotYaw = 0.0f;
		float quatX, quatY, quatZ, quatW = 0.0f;
		Vector translationVector = Vector(0.0f, 0.0f, 0.0f);
		Vector scaleVector = Vector(1.0f, 1.0f, 1.0f);
		Vector rotationVector = Vector(0.0f, 0.0f, 0.0f);
		float CapColorR, CapColorG, CapColorB = 1.0f;
		float ShirtColorR, ShirtColorG, ShirtColorB = 1.0f;
		bool updateVertices = false;
		Vector camLocation, camRotation;
		float fov;
		size_t numTrianglesUsed;
		bool showAltTexture;
	} Frame;

	Model(std::string path, bool inRenderAlways = false);
	Model(std::vector<std::string> meshPaths, bool inRenderAlways = false);
	Model(size_t inMaxTriangles,
		uint8_t* inTexture,
		uint8_t* inAltTexture,
		size_t inTexSize,
		uint16_t inTexWidth,
		uint16_t inTexHeight,
		bool inRenderAlways = false,
		bool noCull = false);
	bool NeedsInitialized();
	bool ShouldRender();
	void InitMeshes(Microsoft::WRL::ComPtr<ID3D11Device> device, int windowWidth, int windowHeight);

	// Model loading
	bool LoadModel();
	void ProcessNode(aiNode* node, const aiScene* scene);
	void ProcessMesh(aiMesh* mesh, const aiScene* scene, const aiNode* node);

	// Model Manipulation
	void Render(CameraWrapper* camera);
	void RenderUpdateVertices(size_t numTrianglesUsed, CameraWrapper* camera);
	void SetTranslation(float x, float y, float z);
	void SetScale(float x, float y, float z);
	void SetRotation(float roll, float pitch, float yaw);
	void SetRotationQuat(float x, float y, float z, float w);
	void SetCapColor(float r, float g, float b);
	void SetShirtColor(float r, float g, float b);
	void SetShowAltTexture(bool val);

	void SetFrame(Frame* frame);
	std::vector<Vertex>* GetVertices();
	std::vector<Frame>* GetFrames();
private:
	void pushRenderFrame(bool updateVertices, CameraWrapper* camera);

public:
	std::vector<Mesh*> Meshes;
	std::vector<std::vector<Vertex>> modelVerticesArr;
	std::vector<std::vector<UINT>> modelIndicesArr;
	std::counting_semaphore<1> sema{ 1 };
	bool backgroundDataLoaded = false;
	std::vector<Frame> Frames;
	bool Disabled = false;
	bool NoCull = false;
private:
	bool meshesInitialized = false;
	std::string modelPath;
	Frame currentFrame;
	bool renderAlways = false;

	// Single mesh init vals
	size_t maxTriangles = 0;
	uint8_t* texture = nullptr;
	uint8_t* altTexture = nullptr;
	size_t texSize = 0;
	uint16_t texWidth = 0;
	uint16_t texHeight = 0;

};