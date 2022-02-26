#pragma once
#include <DirectXMath.h>

#define MAX_LIGHTS 16

struct Vertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT4 color;
	DirectX::XMFLOAT2 texCoord;
	DirectX::XMFLOAT3 normal;
};
typedef struct VS_ConstantBufferData_t
{
	DirectX::XMMATRIX wvp = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
} VS_ConstantBufferData;

typedef struct PS_ConstantBufferData_t
{
	DirectX::XMFLOAT3 ambientLightColor;
	float ambientLightStrength = 0.7f;

	DirectX::XMFLOAT4 dynamicLightColorStrengths[MAX_LIGHTS];
	DirectX::XMFLOAT4 dynamicLightPositions[MAX_LIGHTS];
} PS_ConstantBufferData;

typedef struct Light_t
{
	float r, g, b = 1.0f;
	float posX, posY, posZ = 0.0f;
	float strength = 0.0f;
	bool showBulb = false;
} Light;