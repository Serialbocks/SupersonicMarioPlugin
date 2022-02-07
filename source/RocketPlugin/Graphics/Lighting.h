#pragma once

#include "GraphicsTypes.h"

#define MAX_LIGHTS 8

class Lighting
{
public:
	Lighting();
	void UpdateLights(PS_ConstantBufferData* constBuffer);
	float AmbientLightColorR, AmbientLightColorG, AmbientLightColorB = 1.0f;
	float AmbientLightStrength = 0.7f;
	Light Lights[MAX_LIGHTS];
};

