#include "Model.h"

Model* self = nullptr;

void backgroundLoadData()
{
	self->LoadModel();

	self->sema.acquire();
	self->backgroundDataLoaded = true;
	self->sema.release();
}

Model::Model(std::string path)
{
	self = this;
	modelPath = path;
	std::thread loadMeshesThread(backgroundLoadData);
	loadMeshesThread.detach();
	Renderer::getInstance().AddModel(this);
}

Model::Model(size_t inMaxTriangles,
	uint8_t* inTexture,
	uint8_t* inAltTexture,
	size_t inTexSize,
	uint16_t inTexWidth,
	uint16_t inTexHeight)
{
	maxTriangles = inMaxTriangles;
	texture = inTexture;
	texSize = inTexSize;
	texWidth = inTexWidth;
	texHeight = inTexHeight;
	backgroundDataLoaded = true;
	Renderer::getInstance().AddModel(this);
}

bool Model::NeedsInitialized()
{
	sema.acquire();
	bool needsInit = backgroundDataLoaded && !meshesInitialized;
	sema.release();
	return needsInit;
}

bool Model::ShouldRender()
{
	sema.acquire();
	bool shouldRender = meshesInitialized;
	sema.release();
	return shouldRender;
}

void Model::InitMeshes(Microsoft::WRL::ComPtr<ID3D11Device> device, int windowWidth, int windowHeight)
{
	if (modelVerticesArr.size() == 0)
	{
		Mesh* newMesh = new Mesh(device, windowWidth, windowHeight, maxTriangles, texture, altTexture, texSize, texWidth, texHeight);
		Meshes.push_back(newMesh);
	}
	else
	{
		for (int i = 0; i < modelVerticesArr.size(); i++)
		{
			std::vector<Vertex>* vertices = &modelVerticesArr[i];
			std::vector<UINT>* indices = &modelIndicesArr[i];
			Mesh* newMesh = new Mesh(device, windowWidth, windowHeight, vertices, indices, texture, texSize, texWidth, texHeight);
			Meshes.push_back(newMesh);
		}
	}

	meshesInitialized = true;
}

bool Model::LoadModel()
{
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);
	if (scene == nullptr)
		return false;

	ProcessNode(scene->mRootNode, scene);

	return true;
}

void Model::ProcessNode(aiNode* node, const aiScene* scene)
{
	for (UINT i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(mesh, scene);
	}

	for (UINT i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(node->mChildren[i], scene);
	}
}

void Model::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<Vertex> vertices;
	std::vector<UINT> indices;

	for (UINT i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;

		vertex.pos.x = mesh->mVertices[i].x;
		vertex.pos.y = mesh->mVertices[i].y;
		vertex.pos.z = mesh->mVertices[i].z;

		vertex.normal.x = mesh->mNormals[i].x;
		vertex.normal.y = mesh->mNormals[i].y;
		vertex.normal.z = mesh->mNormals[i].z;

		vertex.color.x = 1.0f;
		vertex.color.y = 1.0f;
		vertex.color.z = 1.0f;
		vertex.color.w = 0.0f;

		vertex.texCoord.x = 1.0f;
		vertex.texCoord.y = 0.0f;

		vertices.push_back(vertex);
	}

	for (UINT i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];

		for (UINT j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	modelVerticesArr.push_back(vertices);
	modelIndicesArr.push_back(indices);
}

void Model::Render(CameraWrapper* camera)
{
	for (auto i = 0; i < Meshes.size(); i++)
	{
		Meshes[i]->Render(camera);
	}
}

void Model::RenderUpdateVertices(size_t numTrianglesUsed, CameraWrapper* camera)
{
	for (auto i = 0; i < Meshes.size(); i++)
	{
		Meshes[i]->RenderUpdateVertices(numTrianglesUsed, camera);
	}
}

void Model::SetTranslation(float x, float y, float z)
{
	for (auto i = 0; i < Meshes.size(); i++)
	{
		Meshes[i]->SetTranslation(x, y, z);
	}
}

void Model::SetScale(float x, float y, float z)
{
	for (auto i = 0; i < Meshes.size(); i++)
	{
		Meshes[i]->SetScale(x, y, z);
	}
}

void Model::SetRotation(float roll, float pitch, float yaw)
{
	for (auto i = 0; i < Meshes.size(); i++)
	{
		Meshes[i]->SetScale(roll, pitch, yaw);
	}
}

void Model::SetRotationQuat(float x, float y, float z, float w)
{
	for (auto i = 0; i < Meshes.size(); i++)
	{
		Meshes[i]->SetRotationQuat(x, y, z, w);
	}
}

void Model::SetCapColor(float r, float g, float b)
{
	for (auto i = 0; i < Meshes.size(); i++)
	{
		Meshes[i]->SetCapColor(r, g, b);
	}
}

void Model::SetShirtColor(float r, float g, float b)
{
	for (auto i = 0; i < Meshes.size(); i++)
	{
		Meshes[i]->SetShirtColor(r, g, b);
	}
}

void Model::SetShowAltTexture(bool val)
{
	for (auto i = 0; i < Meshes.size(); i++)
	{
		Meshes[i]->SetShowAltTexture(val);
	}
}

std::vector<Vertex>* Model::GetVertices()
{
	if (Meshes.size() == 0)
	{
		return nullptr;
	}
	return &Meshes[0]->Vertices;
}
