#pragma once
#include <DirectXMath.h>
typedef struct PS_ConstantBufferData_t
{
	DirectX::XMFLOAT3 ambientLightColor;
	float ambientLightStrength = 0.7f;

	DirectX::XMFLOAT3 dynamicLightColor;
	float dynamicLightStrength = 1.0f;
	DirectX::XMFLOAT3 dynamicLightPosition;
} PS_ConstantBufferData;

typedef struct Light_t
{
	float r, g, b = 1.0f;
	float posX, posY, posZ = 0.0f;
	float strength = 0.0f;
	bool showBulb = false;
} Light;