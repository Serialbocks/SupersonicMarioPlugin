#include "Model.h"

Model* self = nullptr;

void backgroundLoadData()
{
	// For now, with an obj file, assume only one mesh
	self->modelVerticesArr.resize(1);
	self->modelIndicesArr.resize(1);

	self->ParseObjFile(&self->modelVerticesArr[0], &self->modelIndicesArr[0]);

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

void Model::ParseObjFile(std::vector<Vertex>* vertices, std::vector<UINT>* indices)
{
	std::ifstream file(modelPath);
	std::string line;

	std::vector<Vertex> normals;
	while (std::getline(file, line))
	{
		if (line.size() == 0) continue;
		auto split = Utils::SplitStr(line, ' ');
		auto type = split[0];
		auto lastIndex = split.size() - 1;
		if (type == "v")
		{
			Vertex vertex;
			vertex.pos.x = std::stof(split[lastIndex - 2], nullptr);
			vertex.pos.y = std::stof(split[lastIndex - 1], nullptr);
			vertex.pos.z = std::stof(split[lastIndex], nullptr);
			vertex.color.x = 1.0f;
			vertex.color.y = 1.0f;
			vertex.color.z = 1.0f;
			vertex.color.w = 0.0f;
			vertex.texCoord.x = 1.0f;
			vertex.texCoord.y = 0.0f;
			vertices->push_back(vertex);
		}
		else if (type == "vn")
		{
			Vertex vertex;
			vertex.normal.x = std::stof(split[lastIndex - 2], nullptr);
			vertex.normal.y = std::stof(split[lastIndex - 1], nullptr);
			vertex.normal.z = std::stof(split[lastIndex], nullptr);
			normals.push_back(vertex);
		}
		else if (type == "f")
		{
			auto vertIndex1 = std::stoul(Utils::SplitStr(split[lastIndex - 2], '/')[0], nullptr);
			auto vertIndex2 = std::stoul(Utils::SplitStr(split[lastIndex - 1], '/')[0], nullptr);
			auto vertIndex3 = std::stoul(Utils::SplitStr(split[lastIndex], '/')[0], nullptr);
			auto normalIndex1 = std::stoul(Utils::SplitStr(split[lastIndex - 2], '/')[2], nullptr);
			auto normalIndex2 = std::stoul(Utils::SplitStr(split[lastIndex - 1], '/')[2], nullptr);
			auto normalIndex3 = std::stoul(Utils::SplitStr(split[lastIndex], '/')[2], nullptr);
			(*vertices)[vertIndex1].normal.x = normals[normalIndex1].normal.x;
			(*vertices)[vertIndex1].normal.y = normals[normalIndex1].normal.y;
			(*vertices)[vertIndex1].normal.z = normals[normalIndex1].normal.z;
			(*vertices)[vertIndex2].normal.x = normals[normalIndex2].normal.x;
			(*vertices)[vertIndex2].normal.y = normals[normalIndex2].normal.y;
			(*vertices)[vertIndex2].normal.z = normals[normalIndex2].normal.z;
			(*vertices)[vertIndex3].normal.x = normals[normalIndex3].normal.x;
			(*vertices)[vertIndex3].normal.y = normals[normalIndex3].normal.y;
			(*vertices)[vertIndex3].normal.z = normals[normalIndex3].normal.z;
			indices->push_back(vertIndex1);
			indices->push_back(vertIndex2);
			indices->push_back(vertIndex3);
		}
	}

}