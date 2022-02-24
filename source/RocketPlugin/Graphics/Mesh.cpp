#include "Mesh.h"

#define NEAR_Z 50.0f
#define FAR_Z 20000.0f

Mesh::Mesh(Microsoft::WRL::ComPtr<ID3D11Device> deviceIn,
	int inWindowWidth,
	int inWindowHeight,
	size_t maxTriangles,
	uint8_t* inTexture,
	size_t inTexSize,
	uint16_t inTexWidth,
	uint16_t inTexHeight)
{
	init(deviceIn, inWindowWidth, inWindowHeight, maxTriangles, inTexture, inTexSize, inTexWidth, inTexHeight);
}

Mesh::Mesh(Microsoft::WRL::ComPtr<ID3D11Device> deviceIn,
	int inWindowWidth,
	int inWindowHeight,
	std::string objFileName,
	uint8_t* inTexture,
	size_t inTexSize,
	uint16_t inTexWidth,
	uint16_t inTexHeight)
{
	IsTransparent = true;
	parseObjFile(objFileName);
	init(deviceIn, inWindowWidth, inWindowHeight, Vertices.size(), inTexture, inTexSize, inTexWidth, inTexHeight);
	NumTrianglesUsed = Vertices.size();
}

void Mesh::RenderUpdateVertices(size_t numTrianglesUsed, CameraWrapper* camera)
{
	if (UpdateVertices) return;

	Render(camera);

	NumTrianglesUsed = numTrianglesUsed;
	UpdateVertices = true;
}
	
void Mesh::Render(CameraWrapper *camera)
{
	if (camera == nullptr)
	{
		return;
	}

	auto camLocation = camera->GetLocation();
	auto camRotationVector = RotatorToVector(camera->GetRotation());

	float aspectRatio = static_cast<float>(this->windowWidth) / static_cast<float>(this->windowHeight);

	float fov = camera->GetFOV();
	float fovRadians = DirectX::XMConvertToRadians(fov);
	float verticalFovRadians = 2 * atan(tan(fovRadians / 2) / aspectRatio);

	DirectX::XMMATRIX identity = DirectX::XMMatrixIdentity();

	DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(scaleVector.X, scaleVector.Y, scaleVector.Z);

	DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(translationVector.X, translationVector.Y, translationVector.Z);

	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(rotPitch, rotYaw, rotRoll);

	DirectX::FXMVECTOR quat = DirectX::XMVectorSet(quatX, quatY, quatZ, quatW);
	DirectX::XMMATRIX quatRotation = DirectX::XMMatrixRotationQuaternion(quat);

	auto camLocationVector = DirectX::XMVectorSet(camLocation.X, camLocation.Y, camLocation.Z, 0.0f);
	auto camTarget = DirectX::XMVectorSet(camRotationVector.X, camRotationVector.Y, camRotationVector.Z, 0.0f);
	camTarget = DirectX::XMVectorAdd(camTarget, camLocationVector);
	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(camLocationVector, camTarget, this->DEFAULT_UP_VECTOR);

	DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(verticalFovRadians, aspectRatio, NEAR_Z, FAR_Z);

	VertexConstBufferData.world = identity * rotation * quatRotation * scale * translation;
	VertexConstBufferData.wvp = VertexConstBufferData.world * view * projection;
	render = true;
}

void Mesh::SetTranslation(float x, float y, float z)
{
	translationVector.X = x;
	translationVector.Y = y;
	translationVector.Z = z;
}

void Mesh::SetScale(float x, float y, float z)
{
	scaleVector.X = x;
	scaleVector.Y = y;
	scaleVector.Z = z;
}

void Mesh::SetRotation(float roll, float pitch, float yaw)
{
	rotRoll = roll;
	rotPitch = pitch;
	rotYaw = yaw;
}

void Mesh::SetRotationQuat(float x, float y, float z, float w)
{
	quatX = x;
	quatY = y;
	quatZ = z;
	quatW = w;
}

void Mesh::parseObjFile(std::string path)
{
	std::ifstream file(path);
	std::string line;

	std::vector<Vertex> vertices;
	std::vector<Vertex> normals;
	while (std::getline(file, line))
	{
		if (line.size() == 0) continue;
		auto split = splitStr(line, ' ');
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
			vertices.push_back(vertex);
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
			auto vertIndex1 = std::stoul(splitStr(split[lastIndex - 2], '/')[0], nullptr);
			auto vertIndex2 = std::stoul(splitStr(split[lastIndex - 1], '/')[0], nullptr);
			auto vertIndex3 = std::stoul(splitStr(split[lastIndex], '/')[0], nullptr);
			auto normalIndex1 = std::stoul(splitStr(split[lastIndex - 2], '/')[2], nullptr);
			auto normalIndex2 = std::stoul(splitStr(split[lastIndex - 1], '/')[2], nullptr);
			auto normalIndex3 = std::stoul(splitStr(split[lastIndex], '/')[2], nullptr);
			vertices[vertIndex1].normal.x = normals[normalIndex1].normal.x;
			vertices[vertIndex1].normal.y = normals[normalIndex1].normal.y;
			vertices[vertIndex1].normal.z = normals[normalIndex1].normal.z;
			vertices[vertIndex2].normal.x = normals[normalIndex2].normal.x;
			vertices[vertIndex2].normal.y = normals[normalIndex2].normal.y;
			vertices[vertIndex2].normal.z = normals[normalIndex2].normal.z;
			vertices[vertIndex3].normal.x = normals[normalIndex3].normal.x;
			vertices[vertIndex3].normal.y = normals[normalIndex3].normal.y;
			vertices[vertIndex3].normal.z = normals[normalIndex3].normal.z;
			Vertices.push_back(vertices[vertIndex1]);
			Vertices.push_back(vertices[vertIndex2]);
			Vertices.push_back(vertices[vertIndex3]);
		}
	}

}

std::vector<std::string> Mesh::splitStr(std::string str, char delimiter)
{
	std::stringstream stringStream(str);
	std::vector<std::string> seglist;
	std::string segment;
	while (std::getline(stringStream, segment, delimiter))
	{
		seglist.push_back(segment);
	}
	return seglist;
}

void Mesh::init(Microsoft::WRL::ComPtr<ID3D11Device> deviceIn,
	int inWindowWidth,
	int inWindowHeight,
	size_t maxTriangles,
	uint8_t* inTexture,
	size_t inTexSize,
	uint16_t inTexWidth,
	uint16_t inTexHeight)
{
	device = deviceIn;
	windowWidth = inWindowWidth;
	windowHeight = inWindowHeight;

	MaxTriangles = maxTriangles;
	NumIndices = maxTriangles * 3;

	Vertices.resize(NumIndices);
	Indices.resize(NumIndices);
	for (unsigned int i = 0; i < NumIndices; i++)
	{
		Indices[i] = i;
	}

	texData = inTexture;
	texSize = inTexSize;
	texWidth = inTexWidth;
	texHeight = inTexHeight;

	D3D11_BUFFER_DESC vbDesc = { 0 };
	ZeroMemory(&vbDesc, sizeof(D3D11_BUFFER_DESC));
	vbDesc.ByteWidth = (UINT)(sizeof(Vertex) * MaxTriangles * 3);
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.Usage = D3D11_USAGE_DYNAMIC;
	vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vbDesc.StructureByteStride = sizeof(Vertex);

	D3D11_SUBRESOURCE_DATA vbData = { Vertices.data(), 0, 0 };

	device->CreateBuffer(&vbDesc, &vbData, VertexBuffer.GetAddressOf());

	D3D11_BUFFER_DESC ibDesc;
	ZeroMemory(&ibDesc, sizeof(ibDesc));
	ibDesc.ByteWidth = (UINT)(sizeof(unsigned int) * NumIndices);
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA ibData = { Indices.data(), 0, 0 };

	device->CreateBuffer(&ibDesc, &ibData, IndexBuffer.GetAddressOf());

	// Create constant buffer
	// We need to send the world view projection (WVP) matrix to the shader
	D3D11_BUFFER_DESC cbDesc = { 0 };
	ZeroMemory(&cbDesc, sizeof(D3D11_BUFFER_DESC));
	cbDesc.ByteWidth = static_cast<UINT>(sizeof(VS_ConstantBufferData) + (16 - (sizeof(VS_ConstantBufferData) % 16)));
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA cbData = { &VertexConstBufferData, 0, 0 };

	device->CreateBuffer(&cbDesc, &cbData, VertexConstantBuffer.GetAddressOf());


	// If there's texture data, create a shader resource view for it
	if (texData != nullptr)
	{
		D3D11_SUBRESOURCE_DATA subresourceData;
		subresourceData.pSysMem = texData;
		subresourceData.SysMemPitch = 4 * texWidth;
		subresourceData.SysMemSlicePitch = (UINT)texSize;

		D3D11_TEXTURE2D_DESC texture2dDesc;
		texture2dDesc.Width = texWidth;
		texture2dDesc.Height = texHeight;
		texture2dDesc.MipLevels = 1;
		texture2dDesc.ArraySize = 1;
		texture2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texture2dDesc.SampleDesc.Count = 1;
		texture2dDesc.SampleDesc.Quality = 0;
		texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
		texture2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texture2dDesc.CPUAccessFlags = 0;
		texture2dDesc.MiscFlags = 0;

		ID3D11Texture2D* texture;
		device->CreateTexture2D(&texture2dDesc, &subresourceData, &texture);

		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
		memset(&shaderResourceViewDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;

		if (texture != nullptr)
		{
			device->CreateShaderResourceView(texture, &shaderResourceViewDesc, TextureResourceView.GetAddressOf());
		}

	}
	else if (IsTransparent)
	{
		std::string texturePath = utils.GetBakkesmodFolderPath() + "data\\assets\\transparent.png";
		std::wstring texturePathWide(texturePath.begin(), texturePath.end());
		DirectX::CreateWICTextureFromFile(this->device.Get(), texturePathWide.c_str(), nullptr, TextureResourceView.GetAddressOf());
	}
}