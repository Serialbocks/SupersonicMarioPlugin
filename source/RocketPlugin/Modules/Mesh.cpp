#include "Mesh.h"

#define NEAR_Z 0.1f
#define FAR_Z 3000.0f

Mesh::Mesh(Microsoft::WRL::ComPtr<ID3D11Device> deviceIn,
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
	cbDesc.ByteWidth = sizeof(ConstantBufferData);
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA cbData = { &ConstBufferData, 0, 0 };

	device->CreateBuffer(&cbDesc, &cbData, ConstantBuffer.GetAddressOf());

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
}

void Mesh::Render(
	size_t numTrianglesUsed,
	CameraWrapper camera,
	Vector carLoc)
{
	if (RenderFrame) return;

	auto cameraLoc = camera.GetLocation();
	float fov = camera.GetFOV();
	float fovRadians = (fov / 360.0f) * DirectX::XM_2PI;
	float aspectRatio = static_cast<float>(this->windowWidth) / static_cast<float>(this->windowHeight);

	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
	DirectX::XMVECTOR eyePos = DirectX::XMVectorSet(cameraLoc.X, cameraLoc.Z, cameraLoc.Y, 0.0f);
	DirectX::XMVECTOR lookAtPos = DirectX::XMVectorSet(carLoc.X, carLoc.Y, carLoc.Z, 0.0f);
	DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(eyePos, lookAtPos, upVector);
	DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(fovRadians, aspectRatio, NEAR_Z, FAR_Z);

	ConstBufferData.wvp = world * view * projection;
	ConstBufferData.wvp = DirectX::XMMatrixTranspose(ConstBufferData.wvp);

	NumTrianglesUsed = numTrianglesUsed;
	RenderFrame = true;
}