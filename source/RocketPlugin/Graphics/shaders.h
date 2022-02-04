#pragma once

constexpr const char* shaderData = R"(

Texture2D tex;
SamplerState sampleType;

cbuffer constantBuffer
{
    matrix wvp;
};

struct VS_Input
{
    float4 pos : POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

struct VS_Output
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

VS_Output VS(VS_Input input)
{
    VS_Output vsout;
    vsout.pos = mul(input.pos, wvp);
    vsout.color = input.color;
    vsout.texcoord = input.texcoord;
    return vsout;
}

float4 PSTex(VS_Output input) : SV_Target
{
    float4 textureColor;
    textureColor = tex.Sample(sampleType, input.texcoord);
    
    float4 mixedColor = lerp(input.color, textureColor, textureColor.w < 0.3f ? 0.0f : textureColor.w);
    if(input.color.w == 0.0f && textureColor.w == 0.0f)
    {
        discard;
    }
    else
    {
        mixedColor.w = 1.0f;
    }
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