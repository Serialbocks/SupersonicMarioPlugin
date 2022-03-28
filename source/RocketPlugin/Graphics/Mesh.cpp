#include "Mesh.h"

#define NEAR_Z 50.0f
#define FAR_Z 20000.0f
#define MAX_NAMEPLATE_TRIANGLES 300
#define MAX_NAMEPLATE_INDICES (MAX_NAMEPLATE_TRIANGLES * 3)
#define MAX_NAMEPLATE_VERTICES MAX_NAMEPLATE_INDICES

Mesh::Mesh(Microsoft::WRL::ComPtr<ID3D11Device> deviceIn,
	std::shared_ptr<DirectX::SpriteFont> inSpriteFont,
	int inWindowWidth,
	int inWindowHeight,
	size_t maxTriangles,
	uint8_t* inTexture,
	size_t inTexSize,
	uint16_t inTexWidth,
	uint16_t inTexHeight)
{
	init(deviceIn, inSpriteFont, inWindowWidth, inWindowHeight, maxTriangles, inTexture, inTexSize, inTexWidth, inTexHeight);
}

Mesh::Mesh(Microsoft::WRL::ComPtr<ID3D11Device> deviceIn,
	std::shared_ptr<DirectX::SpriteFont> inSpriteFont,
	int inWindowWidth,
	int inWindowHeight,
	std::vector<Vertex> *inVertices,
	uint8_t* inTexture,
	size_t inTexSize,
	uint16_t inTexWidth,
	uint16_t inTexHeight)
{
	IsTransparent = true;
	for (int i = 0; i < inVertices->size(); i++)
	{
		Vertices.push_back((*inVertices)[i]);
	}
	init(deviceIn, inSpriteFont, inWindowWidth, inWindowHeight, Vertices.size(), inTexture, inTexSize, inTexWidth, inTexHeight);
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

	if (spriteFont != nullptr)
	{
		NameplateVertexConstBufferData.world = identity * scale * translation;
		NameplateVertexConstBufferData.wvp = VertexConstBufferData.world * view * projection;
	}

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

void Mesh::SetCapColor(float r, float g, float b)
{
	CapColorR = r;
	CapColorG = g;
	CapColorB = b;
}

void Mesh::SetShirtColor(float r, float g, float b)
{
	ShirtColorR = r;
	ShirtColorG = g;
	ShirtColorB = b;
}

static volatile float nameplateWidth = 500.0f;
static volatile float nameplateHeight = 75.0f;
static volatile float nameplateZOffset = 250.0f;
void Mesh::ShowNameplate(std::wstring name, Vector pos)
{
	Vertex* v1 = &NameplateVertices[0];
	Vertex* v2 = &NameplateVertices[1];
	Vertex* v3 = &NameplateVertices[2];
	Vertex* v4 = &NameplateVertices[3];

	v1->pos.x = -(nameplateWidth / 2);
	v1->pos.y = 0.0f;
	v1->pos.z = (nameplateHeight / 2) + nameplateZOffset;
	v1->color.x = 1.0f;
	v1->color.y = 1.0f;
	v1->color.z = 1.0f;
	v1->color.w = 1.0f;

	v2->pos.x = (nameplateWidth / 2);
	v2->pos.y = 0.0f;
	v2->pos.z = (nameplateHeight / 2) + nameplateZOffset;
	v2->color.x = 1.0f;
	v2->color.y = 1.0f;
	v2->color.z = 1.0f;
	v2->color.w = 1.0f;

	v3->pos.x = -(nameplateWidth / 2);
	v3->pos.y = 0.0f;
	v3->pos.z = -(nameplateHeight / 2) + nameplateZOffset;
	v3->color.x = 1.0f;
	v3->color.y = 1.0f;
	v3->color.z = 1.0f;
	v3->color.w = 1.0f;

	v4->pos.x = (nameplateWidth / 2);
	v4->pos.y = 0.0f;
	v4->pos.z = -(nameplateHeight / 2) + nameplateZOffset;
	v4->color.x = 1.0f;
	v4->color.y = 1.0f;
	v4->color.z = 1.0f;
	v4->color.w = 1.0f;

	NameplateIndices[0] = 0;
	NameplateIndices[1] = 1;
	NameplateIndices[2] = 3;
	NameplateIndices[3] = 0;
	NameplateIndices[4] = 3;
	NameplateIndices[5] = 2;

	NumNameplateTriangles = 2;
}

void Mesh::HideNameplate()
{
	NumNameplateTriangles = 0;
}

void Mesh::init(Microsoft::WRL::ComPtr<ID3D11Device> deviceIn,
	std::shared_ptr<DirectX::SpriteFont> inSpriteFont,
	int inWindowWidth,
	int inWindowHeight,
	size_t maxTriangles,
	uint8_t* inTexture,
	size_t inTexSize,
	uint16_t inTexWidth,
	uint16_t inTexHeight)
{
	device = deviceIn;
	spriteFont = inSpriteFont;
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


	if (inSpriteFont != nullptr)
	{
		NameplateVertices.resize(MAX_NAMEPLATE_VERTICES);
		NameplateIndices.resize(MAX_NAMEPLATE_INDICES);

		// Create buffers for nameplate
		ZeroMemory(&vbDesc, sizeof(D3D11_BUFFER_DESC));
		vbDesc.ByteWidth = (UINT)(sizeof(Vertex) * MAX_NAMEPLATE_VERTICES);
		vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbDesc.Usage = D3D11_USAGE_DYNAMIC;
		vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		vbDesc.StructureByteStride = sizeof(Vertex);

		D3D11_SUBRESOURCE_DATA npvbData = { NameplateVertices.data(), 0, 0 };

		device->CreateBuffer(&vbDesc, &npvbData, NameplateVertexBuffer.GetAddressOf());

		auto numNameplateIndices = MAX_NAMEPLATE_INDICES;
		ZeroMemory(&ibDesc, sizeof(ibDesc));
		ibDesc.ByteWidth = (UINT)(sizeof(unsigned int) * numNameplateIndices);
		ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibDesc.Usage = D3D11_USAGE_DEFAULT;
		ibDesc.CPUAccessFlags = 0;
		ibDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA npibData = { NameplateIndices.data(), 0, 0 };

		device->CreateBuffer(&ibDesc, &npibData, NameplateIndexBuffer.GetAddressOf());

		ZeroMemory(&cbDesc, sizeof(D3D11_BUFFER_DESC));
		cbDesc.ByteWidth = static_cast<UINT>(sizeof(VS_ConstantBufferData) + (16 - (sizeof(VS_ConstantBufferData) % 16)));
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbDesc.MiscFlags = 0;
		cbDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA npcbData = { &NameplateVertexConstBufferData, 0, 0 };

		device->CreateBuffer(&cbDesc, &npcbData, NameplateVertexConstantBuffer.GetAddressOf());
	}


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