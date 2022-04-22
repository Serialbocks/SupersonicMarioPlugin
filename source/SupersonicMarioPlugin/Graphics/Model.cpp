#include "Model.h"

void backgroundLoadData(Model* self)
{
	self->LoadModel();

	self->sema.acquire();
	self->backgroundDataLoaded = true;
	self->sema.release();
}

Model::Model(std::string path)
{
	modelPath = path;
	std::thread loadMeshesThread(backgroundLoadData, this);
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
	bool shouldRender = meshesInitialized && backgroundDataLoaded;
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
		ProcessMesh(mesh, scene, node);
	}

	for (UINT i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(node->mChildren[i], scene);
	}
}

void Model::ProcessMesh(aiMesh* mesh, const aiScene* scene, const aiNode* node)
{
	std::vector<Vertex> vertices;
	std::vector<UINT> indices;

	for (UINT i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;

		aiVector3D aiVertex = node->mTransformation * mesh->mVertices[i];

		vertex.pos.x = aiVertex.x;
		vertex.pos.y = aiVertex.y;
		vertex.pos.z = aiVertex.z;

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

void Model::pushRenderFrame(bool updateVertices, CameraWrapper* camera)
{
	if (camera != nullptr)
	{
		currentFrame.updateVertices = updateVertices;
		currentFrame.camLocation = camera->GetLocation();
		currentFrame.camRotation = RotatorToVector(camera->GetRotation());
		currentFrame.fov = camera->GetFOV();
		Frames.push_back(currentFrame);
	}

}

void Model::Render(CameraWrapper* camera)
{
	pushRenderFrame(false, camera);
}

void Model::RenderUpdateVertices(size_t numTrianglesUsed, CameraWrapper* camera)
{
	currentFrame.numTrianglesUsed = numTrianglesUsed;
	pushRenderFrame(true, camera);
}

void Model::SetTranslation(float x, float y, float z)
{
	currentFrame.translationVector.X = x;
	currentFrame.translationVector.Y = y;
	currentFrame.translationVector.Z = z;
}

void Model::SetScale(float x, float y, float z)
{
	currentFrame.scaleVector.X = x;
	currentFrame.scaleVector.Y = y;
	currentFrame.scaleVector.Z = z;
}

void Model::SetRotation(float roll, float pitch, float yaw)
{
	currentFrame.rotRoll = roll;
	currentFrame.rotPitch = pitch;
	currentFrame.rotYaw = yaw;
}

void Model::SetRotationQuat(float x, float y, float z, float w)
{
	currentFrame.quatX = x;
	currentFrame.quatY = y;
	currentFrame.quatZ = z;
	currentFrame.quatW = w;
}

void Model::SetCapColor(float r, float g, float b)
{
	currentFrame.CapColorR = r;
	currentFrame.CapColorG = g;
	currentFrame.CapColorB = b;
}

void Model::SetShirtColor(float r, float g, float b)
{
	currentFrame.ShirtColorR = r;
	currentFrame.ShirtColorG = g;
	currentFrame.ShirtColorB = b;
}

void Model::SetShowAltTexture(bool val)
{
	currentFrame.showAltTexture = val;
}

void Model::SetFrame(Frame* frame)
{
	for (auto i = 0; i < Meshes.size(); i++)
	{
		Mesh* mesh = Meshes[i];

		mesh->SetTranslation(frame->translationVector.X,
			frame->translationVector.Y,
			frame->translationVector.Z);
		mesh->SetScale(frame->scaleVector.X,
			frame->scaleVector.Y,
			frame->scaleVector.Z);
		mesh->SetRotation(frame->rotRoll,
			frame->rotPitch,
			frame->rotYaw);
		mesh->SetRotationQuat(frame->quatX,
			frame->quatY,
			frame->quatZ,
			frame->quatW);
		mesh->SetCapColor(frame->CapColorR,
			frame->CapColorG,
			frame->CapColorB);
		mesh->SetShirtColor(frame->ShirtColorR,
			frame->ShirtColorG,
			frame->ShirtColorB);
		mesh->SetShowAltTexture(frame->showAltTexture);

		if (frame->updateVertices)
		{
			mesh->RenderUpdateVertices(frame->numTrianglesUsed,
				frame->camLocation,
				frame->camRotation,
				frame->fov);
		}
		else
		{
			mesh->Render(frame->camLocation,
				frame->camRotation,
				frame->fov);
		}
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

std::vector<Model::Frame>* Model::GetFrames()
{
	if (Frames.size() == 0)
	{
		Frames.push_back(currentFrame);
	}
	return &Frames;
}
