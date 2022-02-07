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
	Lights[0].strength = 1.0f;
	Lights[0].posX = 0.0f;
	Lights[0].posY = 83.0f;
	Lights[0].posZ = 2000.0f;
}

void Lighting::UpdateLights(PS_ConstantBufferData* constBufferData)
{
	constBufferData->ambientLightColor.x = AmbientLightColorR;
	constBufferData->ambientLightColor.y = AmbientLightColorG;
	constBufferData->ambientLightColor.z = AmbientLightColorB;
	constBufferData->ambientLightStrength = AmbientLightStrength;
	constBufferData->dynamicLightColor.x = Lights[0].r;
	constBufferData->dynamicLightColor.y = Lights[0].g;
	constBufferData->dynamicLightColor.z = Lights[0].b;
	constBufferData->dynamicLightStrength = Lights[0].strength;
	constBufferData->dynamicLightPosition.x = Lights[0].posX;
	constBufferData->dynamicLightPosition.y = Lights[0].posY;
	constBufferData->dynamicLightPosition.z = Lights[0].posZ;
}
