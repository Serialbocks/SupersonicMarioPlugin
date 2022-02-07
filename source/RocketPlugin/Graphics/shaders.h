#pragma once

constexpr const char* shaderData = R"(
Texture2D tex;
SamplerState sampleType;

#pragma pack_matrix( row_major )

cbuffer VS_constantBuffer
{
    matrix wvp;
    matrix world;
};

struct VS_Input
{
    float4 pos : POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
};

struct VS_Output
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 worldPos : WORLD_POSITION;
};

VS_Output VS(VS_Input input)
{
    VS_Output vsout;
    vsout.pos = mul(input.pos, wvp);
    vsout.color = input.color;
    vsout.texcoord = input.texcoord;
    vsout.normal = (float3)normalize(mul(float4(input.normal, 0.0f), world));
    vsout.worldPos = (float3)mul(input.pos, world);
    return vsout;
}

static const int numLights = 16;
cbuffer PS_constantBuffer
{
    float3 ambientLightColor;
    float ambientLightIntensity;

    float4 dynamicLightColorStrengths[numLights];
    float4 dynamicLightPositions[numLights];
};

float4 PSTex(VS_Output input) : SV_Target
{
    float4 textureColor;
    textureColor = tex.Sample(sampleType, input.texcoord);
    
    if(input.color.w == 0.0f && textureColor.w == 0.0f)
    {
        discard;
    }
    float4 mixedColor = lerp(input.color, textureColor, textureColor.w < 0.3f ? 0.0f : textureColor.w);

    if(input.color.w > 0.0f)
    {
        float3 ambientLight = ambientLightColor * ambientLightIntensity;
        float3 appliedLight = ambientLight;
        for(int i = 0; i < numLights; i++)
        {
            float3 dynamicLightPosition = dynamicLightPositions[i].rgb;
            float3 dynamicLightColor = dynamicLightColorStrengths[i].rgb;
            float dynamicLightStrength = dynamicLightColorStrengths[i].w;

            float3 vectorToLight = normalize(dynamicLightPosition - input.worldPos);
            float3 diffuseLightIntensity = max(dot(vectorToLight, input.normal), 0);
            float3 diffuseLight = diffuseLightIntensity * dynamicLightStrength * dynamicLightColor;
            appliedLight += diffuseLight;
        }

        mixedColor.rgb = mixedColor.rgb * appliedLight;
    }


    mixedColor.w = 1.0f;

    return mixedColor;
}

float4 PSTexTransparent(VS_Output input) : SV_Target
{
    return tex.Sample(sampleType, input.texcoord);
}

float4 PS(VS_Output input) : SV_Target
{
    return input.color;
})";