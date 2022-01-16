#include "Renderer.h"

using namespace Microsoft::WRL;
using namespace DirectX;

Renderer* instance = nullptr;

#define PRESENT_INDEX 8
typedef HRESULT(__stdcall* Present)(IDXGISwapChain*, UINT, UINT);
static Present oPresent = NULL;
HRESULT __stdcall hkPresent(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags)
{
	if (instance != nullptr)
	{
		instance->OnPresent(pThis, SyncInterval, Flags);
	}
	return oPresent(pThis, SyncInterval, Flags);
}


Renderer::Renderer(uint16_t maxTriangles,
	uint8_t* inTexture,
	size_t inTexSize, uint16_t inTexWidth, uint16_t inTexHeight)
{
	instance = this;
	MaxTriangles = maxTriangles;
	numIndices = maxTriangles * 3;

	vertices = (Vertex*)malloc(sizeof(Vertex) * numIndices);
	ZeroMemory(vertices, sizeof(Vertex) * numIndices);

	indices = (unsigned int*)malloc(sizeof(unsigned int) * numIndices);
	ZeroMemory(indices, sizeof(unsigned int) * numIndices);

	texData = inTexture;
	texSize = inTexSize;
	texWidth = inTexWidth;
	texHeight = inTexHeight;

	for (unsigned int i = 0; i < numIndices; i++)
	{
		indices[i] = i;
	}
	if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
	{
		kiero::bind(PRESENT_INDEX, (void**)&oPresent, hkPresent);
	}
}

Renderer::~Renderer()
{
	kiero::shutdown();
	instance = nullptr;
	free(vertices);
	free(indices);
}

void Renderer::OnPresent(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags)
{
	if (!initialized)
	{
		if (!Init(pThis, SyncInterval, Flags)) return;
		drawExamples = true;
	}

	Render();
}

bool Renderer::Init(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
	if (firstInit)
	{
		HRESULT getDevice = swapChain->GetDevice(__uuidof(ID3D11Device), (void**)&device);

		if (SUCCEEDED(getDevice))
		{
			device->GetImmediateContext(&context);
		}
		else
		{
			return false;
		}

	}

	DXGI_SWAP_CHAIN_DESC desc;
	ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC));
	swapChain->GetDesc(&desc);

	RECT hwndRect;
	GetClientRect(desc.OutputWindow, &hwndRect);
	windowWidth = hwndRect.right - hwndRect.left;
	windowHeight = hwndRect.bottom - hwndRect.top;

	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.Width = (float)windowWidth;
	viewport.Height = (float)windowHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;

	ComPtr<ID3D11Texture2D> backbuffer;
	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backbuffer.GetAddressOf());
	device->CreateRenderTargetView(backbuffer.Get(), nullptr, &mainRenderTargetView);
	backbuffer.ReleaseAndGetAddressOf();

	//overlay.SetWindowHandle(desc.OutputWindow);

	initialized = true;
	firstInit = false;
	return true;
}

void Renderer::InitMeshBuffers()
{
	D3D11_BUFFER_DESC vbDesc = { 0 };
	ZeroMemory(&vbDesc, sizeof(D3D11_BUFFER_DESC));
	vbDesc.ByteWidth = sizeof(Vertex) * MaxTriangles * 3;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.Usage = D3D11_USAGE_DYNAMIC;
	vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vbDesc.StructureByteStride = sizeof(Vertex);

	D3D11_SUBRESOURCE_DATA vbData = { vertices, 0, 0 };

	device->CreateBuffer(&vbDesc, &vbData, vertexBuffer.GetAddressOf());

	D3D11_BUFFER_DESC ibDesc;
	ZeroMemory(&ibDesc, sizeof(ibDesc));
	ibDesc.ByteWidth = sizeof(unsigned int) * numIndices;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA ibData = { indices, 0, 0 };

	device->CreateBuffer(&ibDesc, &ibData, indexBuffer.GetAddressOf());

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

		device->CreateShaderResourceView(texture, &shaderResourceViewDesc, textureResourceView.GetAddressOf());
	}

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

	D3D11_SUBRESOURCE_DATA cbData = { &constantBufferData, 0, 0 };

	device->CreateBuffer(&cbDesc, &cbData, constantBuffer.GetAddressOf());

	// Create rasterizer state
	// We need to control which face of a shape is culled
	// And we need to know which order to set our indices
	D3D11_RASTERIZER_DESC rsDesc;
	ZeroMemory(&rsDesc, sizeof(D3D11_RASTERIZER_DESC));
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_NONE;
	rsDesc.FrontCounterClockwise = FALSE;
	rsDesc.DepthClipEnable = TRUE;

	device->CreateRasterizerState(&rsDesc, rasterizerState.GetAddressOf());

	// Create depth stencil state
	D3D11_DEPTH_STENCIL_DESC dsDesc;
	ZeroMemory(&dsDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	device->CreateDepthStencilState(&dsDesc, depthStencilState.GetAddressOf());
}

// Creates the necessary things for rendering the examples
void Renderer::CreatePipeline()
{
	ComPtr<ID3DBlob> vertexShaderBlob = LoadShader(shaderData, "vs_5_0", "VS").Get();
	ComPtr<ID3DBlob> pixelShaderTexturesBlob = LoadShader(shaderData, "ps_5_0", "PSTex").Get();
	ComPtr<ID3DBlob> pixelShaderBlob = LoadShader(shaderData, "ps_5_0", "PS").Get();

	device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize(), nullptr, vertexShader.GetAddressOf());

	device->CreatePixelShader(pixelShaderTexturesBlob->GetBufferPointer(),
		pixelShaderTexturesBlob->GetBufferSize(), nullptr, pixelShaderTextures.GetAddressOf());

	device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(),
		pixelShaderBlob->GetBufferSize(), nullptr, pixelShader.GetAddressOf());

	D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[3] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	device->CreateInputLayout(inputLayoutDesc, ARRAYSIZE(inputLayoutDesc), vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize(), inputLayout.GetAddressOf());

	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	device->CreateSamplerState(&samplerDesc, samplerState.GetAddressOf());

	// Create depth stencil
	D3D11_TEXTURE2D_DESC dsDesc;
	dsDesc.Width = windowWidth;
	dsDesc.Height = windowHeight;
	dsDesc.MipLevels = 1;
	dsDesc.ArraySize = 1;
	dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsDesc.SampleDesc.Count = 1;
	dsDesc.SampleDesc.Quality = 0;
	dsDesc.Usage = D3D11_USAGE_DEFAULT;
	dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	dsDesc.CPUAccessFlags = 0;
	dsDesc.MiscFlags = 0;

	device->CreateTexture2D(&dsDesc, 0, depthStencilBuffer.GetAddressOf());
	device->CreateDepthStencilView(depthStencilBuffer.Get(), 0, depthStencilView.GetAddressOf());
}

void Renderer::Render()
{
	context->OMSetRenderTargets(1, mainRenderTargetView.GetAddressOf(), 0);
	context->RSSetViewports(1, &viewport);

	if (drawExamples)
	{

		if (!examplesLoaded)
		{
			CreatePipeline();
			InitMeshBuffers();
			examplesLoaded = true;
		}

		DrawRenderedMesh();
	}
}

ComPtr<ID3DBlob> Renderer::LoadShader(const char* shader, std::string targetShaderVersion, std::string shaderEntry)
{
	ComPtr<ID3DBlob> errorBlob = nullptr;
	ComPtr<ID3DBlob> shaderBlob;

	D3DCompile(shader, strlen(shader), 0, nullptr, nullptr, shaderEntry.c_str(), targetShaderVersion.c_str(), D3DCOMPILE_ENABLE_STRICTNESS, 0, shaderBlob.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob)
	{
		char error[256]{ 0 };
		memcpy(error, errorBlob->GetBufferPointer(), errorBlob->GetBufferSize());
		return nullptr;
	}

	return shaderBlob;
}

void Renderer::DrawRenderedMesh()
{
	context->OMSetRenderTargets(1, mainRenderTargetView.GetAddressOf(), depthStencilView.Get());
	context->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Map the constant buffer on the GPU
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	context->Map(constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	memcpy(mappedResource.pData, &constantBufferData, sizeof(ConstantBufferData));
	context->Unmap(constantBuffer.Get(), 0);

	if (renderFrame)
	{
		context->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		memcpy(mappedResource.pData, (void*)vertices, sizeof(Vertex) * NumTrianglesUsed * 3);
		context->Unmap(vertexBuffer.Get(), 0);
		renderFrame = false;
	}

	context->VSSetShader(vertexShader.Get(), nullptr, 0);
	context->PSSetShader(pixelShader.Get(), nullptr, 0);

	context->IASetInputLayout(inputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->RSSetState(rasterizerState.Get());
	context->OMSetDepthStencilState(depthStencilState.Get(), 0);

	context->UpdateSubresource(constantBuffer.Get(), 0, 0, &constantBufferData, 0, 0);
	context->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	if (textureResourceView != nullptr)
	{
		context->PSSetShader(pixelShaderTextures.Get(), nullptr, 0);
		context->PSSetSamplers(0, 1, samplerState.GetAddressOf());
		context->PSSetShaderResources(0, 1, textureResourceView.GetAddressOf());
		context->DrawIndexed(NumTrianglesUsed * 3, 0, 0);
	}
	else
	{
		context->DrawIndexed(NumTrianglesUsed * 3, 0, 0);
	}
}

void Renderer::RenderMesh(
	MeshVertex* inVertices,
	uint16_t numTrianglesUsed)
{
	if (renderFrame) return;
	for (int i = 0; i < numTrianglesUsed * 3; i++)
	{
		auto curVertex = inVertices[i];
		vertices[i].pos.x = (2 * curVertex.position.X / windowWidth) - 1.0f;
		vertices[i].pos.y = (-2 * curVertex.position.Y / windowHeight) + 1.0f;
		vertices[i].pos.z = curVertex.position.Z;
		vertices[i].color.x = curVertex.r;
		vertices[i].color.y = curVertex.g;
		vertices[i].color.z = curVertex.b;
		vertices[i].color.w = curVertex.a;
		vertices[i].texCoord.x = curVertex.u;
		vertices[i].texCoord.y = curVertex.v;
	}

	NumTrianglesUsed = numTrianglesUsed;
	renderFrame = true;
}