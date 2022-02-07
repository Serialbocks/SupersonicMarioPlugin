#include "Lighting.h"
Lighting::Lighting()
{
	AmbientLightColorR = 1.0f;
	AmbientLightColorG = 1.0f;
	AmbientLightColorB = 1.0f;
	AmbientLightStrength = 0.7f;
	Lights[0].r = 1.0f;
	Lights[0].g = 1.0f;
	Lights[0].b = 1.0f;
	Lights[0].strength = 0.3f;
	Lights[0].posX = 0.0f;
	Lights[0].posY = 83.0f;
	Lights[0].posZ = 2000.0f;
	for (auto i = 1; i < MAX_LIGHTS; i++)
	{
		Lights[i].r = 1.0f;
		Lights[i].g = 1.0f;
		Lights[i].b = 1.0f;
		Lights[i].strength = 0.0f;
		Lights[i].posX = 0.0f;
		Lights[i].posY = 0.0f;
		Lights[i].posZ = 50.0f;
	}
}

void Lighting::UpdateLights(PS_ConstantBufferData* constBufferData)
{
	constBufferData->ambientLightColor.x = AmbientLightColorR;
	constBufferData->ambientLightColor.y = AmbientLightColorG;
	constBufferData->ambientLightColor.z = AmbientLightColorB;
	constBufferData->ambientLightStrength = AmbientLightStrength;
	for (auto i = 0; i < MAX_LIGHTS; i++)
	{
		constBufferData->dynamicLightColorStrengths[i].x = Lights[i].r;
		constBufferData->dynamicLightColorStrengths[i].y = Lights[i].g;
		constBufferData->dynamicLightColorStrengths[i].z = Lights[i].b;
		constBufferData->dynamicLightColorStrengths[i].w = Lights[i].strength;
		constBufferData->dynamicLightPositions[i].x = Lights[i].posX;
		constBufferData->dynamicLightPositions[i].y = Lights[i].posY;
		constBufferData->dynamicLightPositions[i].z = Lights[i].posZ;
	}

}
