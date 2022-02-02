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


Renderer::Renderer()
{
	instance = this;
	
	if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
	{
		kiero::bind(PRESENT_INDEX, (void**)&oPresent, hkPresent);
	}
}

Renderer::~Renderer()
{
	kiero::shutdown();
	instance = nullptr;
	for (auto i = 0; i < meshes.size(); i++)
	{
		delete meshes[i];
	}
}

Mesh* Renderer::CreateMesh(size_t maxTriangles,
	uint8_t* inTexture,
	size_t inTexSize,
	uint16_t inTexWidth,
	uint16_t inTexHeight)
{
	if (!device)
	{
		return nullptr;
	}
	Mesh* newMesh = new Mesh(device, windowWidth, windowHeight, maxTriangles, inTexture, inTexSize, inTexWidth, inTexHeight);
	meshes.push_back(newMesh);
	return newMesh;
}

void Renderer::OnPresent(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags)
{
	if (!Initialized)
	{
		if (!Init(pThis, SyncInterval, Flags)) return;
		drawMeshes = true;
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

	Initialized = true;
	firstInit = false;
	return true;
}

void Renderer::InitMeshBuffers()
{
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

	if (drawMeshes)
	{
		if (!pipelineInitialized)
		{
			CreatePipeline();
			InitMeshBuffers();
			pipelineInitialized = true;
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

	context->VSSetShader(vertexShader.Get(), nullptr, 0);
	context->PSSetShader(pixelShader.Get(), nullptr, 0);

	context->IASetInputLayout(inputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->RSSetState(rasterizerState.Get());
	context->OMSetDepthStencilState(depthStencilState.Get(), 0);

	UINT stride = sizeof(Mesh::Vertex);
	UINT offset = 0;

	for (auto i = 0; i < meshes.size(); i++)
	{
		auto mesh = meshes[i];

		// Map the constant buffer on the GPU
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		context->Map(mesh->ConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		memcpy(mappedResource.pData, &mesh->ConstBufferData, sizeof(Mesh::ConstantBufferData));
		context->Unmap(mesh->ConstantBuffer.Get(), 0);

		context->UpdateSubresource(mesh->ConstantBuffer.Get(), 0, 0, &mesh->ConstBufferData, 0, 0);
		context->VSSetConstantBuffers(0, 1, mesh->ConstantBuffer.GetAddressOf());

		if (mesh->RenderFrame)
		{
			context->Map(mesh->VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			memcpy(mappedResource.pData, (void*)mesh->Vertices.data(), sizeof(Mesh::Vertex) * mesh->NumTrianglesUsed * 3);
			context->Unmap(mesh->VertexBuffer.Get(), 0);
			mesh->RenderFrame = false;
		}

		context->IASetVertexBuffers(0, 1, mesh->VertexBuffer.GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(mesh->IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		if (mesh->TextureResourceView != nullptr)
		{
			context->PSSetShader(pixelShaderTextures.Get(), nullptr, 0);
			context->PSSetSamplers(0, 1, samplerState.GetAddressOf());
			context->PSSetShaderResources(0, 1, mesh->TextureResourceView.GetAddressOf());
			context->DrawIndexed((UINT)mesh->NumTrianglesUsed * 3, 0, 0);
		}
		else
		{
			context->PSSetShader(pixelShader.Get(), nullptr, 0);
			context->DrawIndexed((UINT)mesh->NumTrianglesUsed * 3, 0, 0);
		}
	}
}