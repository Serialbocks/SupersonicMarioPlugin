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
}

void Renderer::AddModel(Model* model)
{
	models.push_back(model);
}

void Renderer::OnPresent(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags)
{
	if (!Initialized)
	{
		if (!Init(pThis, SyncInterval, Flags)) return;
		drawModels = true;
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
	Window = desc.OutputWindow;

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

void Renderer::InitBuffers()
{
	// Create rasterizer state
	// We need to control which face of a shape is culled
	// And we need to know which order to set our indices
	D3D11_RASTERIZER_DESC rsDesc;
	ZeroMemory(&rsDesc, sizeof(D3D11_RASTERIZER_DESC));
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_FRONT;
	rsDesc.FrontCounterClockwise = FALSE;
	rsDesc.DepthClipEnable = TRUE;

	device->CreateRasterizerState(&rsDesc, rasterizerState.GetAddressOf());

	// Create depth stencil state
	D3D11_DEPTH_STENCIL_DESC dsDesc;
	ZeroMemory(&dsDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	D3D11_DEPTH_STENCILOP_DESC stencilopDesc;
	ZeroMemory(&stencilopDesc, sizeof(D3D11_DEPTH_STENCILOP_DESC));
	stencilopDesc.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	stencilopDesc.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	stencilopDesc.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	stencilopDesc.StencilFunc = D3D11_COMPARISON_ALWAYS;
	
	dsDesc.FrontFace = stencilopDesc;
	dsDesc.BackFace = stencilopDesc;

	device->CreateDepthStencilState(&dsDesc, depthStencilState.GetAddressOf());
}

// Creates the necessary things for rendering the examples
void Renderer::CreatePipeline()
{
	ComPtr<ID3DBlob> vertexShaderBlob = LoadShader(shaderData, "vs_5_0", "VS").Get();
	ComPtr<ID3DBlob> pixelShaderTexturesBlob = LoadShader(shaderData, "ps_5_0", "PSTex").Get();
	ComPtr<ID3DBlob> pixelShaderTransparentTextureBlox = LoadShader(shaderData, "ps_5_0", "PSTexTransparent").Get();
	ComPtr<ID3DBlob> pixelShaderBlob = LoadShader(shaderData, "ps_5_0", "PS").Get();

	device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize(), nullptr, vertexShader.GetAddressOf());

	device->CreatePixelShader(pixelShaderTexturesBlob->GetBufferPointer(),
		pixelShaderTexturesBlob->GetBufferSize(), nullptr, pixelShaderTextures.GetAddressOf());

	device->CreatePixelShader(pixelShaderTransparentTextureBlox->GetBufferPointer(),
		pixelShaderTransparentTextureBlox->GetBufferSize(), nullptr, pixelShaderTexturesTransparent.GetAddressOf());

	device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(),
		pixelShaderBlob->GetBufferSize(), nullptr, pixelShader.GetAddressOf());

	D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[4] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
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

	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(D3D11_BLEND_DESC));
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;
	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	device->CreateBlendState(&blendDesc, blendState.GetAddressOf());

	D3D11_BUFFER_DESC cbDesc = { 0 };
	cbDesc.ByteWidth = static_cast<UINT>(sizeof(PS_ConstantBufferData) + (16 - (sizeof(PS_ConstantBufferData) % 16)));
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA pixelCbData = { &PixelConstBufferData, 0, 0 };

	device->CreateBuffer(&cbDesc, &pixelCbData, pixelConstantBuffer.GetAddressOf());
}

void Renderer::Render()
{
	context->OMSetRenderTargets(1, mainRenderTargetView.GetAddressOf(), 0);
	context->RSSetViewports(1, &viewport);

	if (drawModels)
	{
		if (!pipelineInitialized)
		{
			CreatePipeline();
			InitBuffers();
			pipelineInitialized = true;
		}

		DrawModels();
	}
}

ComPtr<ID3DBlob> Renderer::LoadShader(const char* shader, std::string targetShaderVersion, std::string shaderEntry)
{
	ComPtr<ID3DBlob> errorBlob = nullptr;
	ComPtr<ID3DBlob> shaderBlob;

	D3DCompile(shader, strlen(shader), 0, nullptr, nullptr, shaderEntry.c_str(), targetShaderVersion.c_str(), D3DCOMPILE_ENABLE_STRICTNESS, 0, shaderBlob.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob)
	{
		static volatile char error[256]{ 0 };
		memcpy((void*)error, errorBlob->GetBufferPointer(), errorBlob->GetBufferSize());
		return nullptr;
	}

	return shaderBlob;
}

void Renderer::DrawModels()
{
	context->OMSetRenderTargets(1, mainRenderTargetView.GetAddressOf(), depthStencilView.Get());
	context->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	context->VSSetShader(vertexShader.Get(), nullptr, 0);
	context->PSSetShader(pixelShader.Get(), nullptr, 0);

	context->IASetInputLayout(inputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->RSSetState(rasterizerState.Get());
	context->OMSetDepthStencilState(depthStencilState.Get(), 0);

	context->OMSetBlendState(blendState.Get(), NULL, 0xFFFFFFFF);

	Lighting.UpdateLights(&PixelConstBufferData);

	context->UpdateSubresource(pixelConstantBuffer.Get(), 0, 0, &PixelConstBufferData, 0, 0);
	context->PSSetConstantBuffers(0, 1, pixelConstantBuffer.GetAddressOf());

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	for (auto k = 0; k < models.size(); k++)
	{
		auto model = models[k];

		if (model->NeedsInitialized())
		{
			model->InitMeshes(device, windowWidth, windowHeight);
		}

		if (!model->ShouldRender())
		{
			continue;
		}

		auto frames = model->GetFrames();
		for (auto m = 0; m < frames->size(); m++)
		{
			auto frame = (*frames)[m];
			model->SetFrame(&frame);

			for (auto i = 0; i < model->Meshes.size(); i++)
			{
				auto mesh = model->Meshes[i];

				if (!mesh->render) {
					mesh->UpdateVertices = false;
					continue;
				};
				//mesh->render = false;

				PixelConstBufferData.capColor.x = mesh->CapColorR;
				PixelConstBufferData.capColor.y = mesh->CapColorG;
				PixelConstBufferData.capColor.z = mesh->CapColorB;
				PixelConstBufferData.shirtColor.x = mesh->ShirtColorR;
				PixelConstBufferData.shirtColor.y = mesh->ShirtColorG;
				PixelConstBufferData.shirtColor.z = mesh->ShirtColorB;

				// Map the pixel constant buffer
				D3D11_MAPPED_SUBRESOURCE mappedResource;
				context->Map(pixelConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
				memcpy(mappedResource.pData, &PixelConstBufferData, sizeof(PS_ConstantBufferData));
				context->Unmap(pixelConstantBuffer.Get(), 0);

				// Map the vertex constant buffer on the GPU
				context->Map(mesh->VertexConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
				memcpy(mappedResource.pData, &mesh->VertexConstBufferData, sizeof(VS_ConstantBufferData));
				context->Unmap(mesh->VertexConstantBuffer.Get(), 0);

				context->UpdateSubresource(mesh->VertexConstantBuffer.Get(), 0, 0, &mesh->VertexConstBufferData, 0, 0);
				context->VSSetConstantBuffers(0, 1, mesh->VertexConstantBuffer.GetAddressOf());

				if (mesh->UpdateVertices)
				{
					context->Map(mesh->VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
					memcpy(mappedResource.pData, (void*)mesh->Vertices.data(), sizeof(Vertex) * mesh->NumTrianglesUsed * 3);
					context->Unmap(mesh->VertexBuffer.Get(), 0);
					mesh->UpdateVertices = false;
				}

				context->IASetVertexBuffers(0, 1, mesh->VertexBuffer.GetAddressOf(), &stride, &offset);
				context->IASetIndexBuffer(mesh->IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

				if (mesh->TextureResourceView != nullptr)
				{
					if (mesh->IsTransparent)
					{
						context->PSSetShader(pixelShaderTexturesTransparent.Get(), nullptr, 0);
					}
					else
					{
						context->PSSetShader(pixelShaderTextures.Get(), nullptr, 0);
					}
					context->PSSetSamplers(0, 1, samplerState.GetAddressOf());
					if (mesh->ShowAltTexture && mesh->AltTextureResourceView != nullptr)
					{
						context->PSSetShaderResources(0, 1, mesh->AltTextureResourceView.GetAddressOf());
					}
					else
					{
						context->PSSetShaderResources(0, 1, mesh->TextureResourceView.GetAddressOf());
					}
				}
				else
				{
					context->PSSetShader(pixelShader.Get(), nullptr, 0);
				}
				context->DrawIndexed((UINT)mesh->NumTrianglesUsed * 3, 0, 0);
			}
		}

		model->Frames.clear();


	}



}