#define rootSignatureBlur "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),\
RootConstants(num32BitConstants=1, b0),\
DescriptorTable( SRV(t0, numDescriptors = 10), visibility=SHADER_VISIBILITY_PIXEL),\
StaticSampler(s0, visibility = SHADER_VISIBILITY_PIXEL)"

#define MaxLights 10
struct Light
{
    float3 Strength;
    float FalloffStart;
    float3 Direction;
    float FalloffEnd;
    float3 Position;
    float SpotPower;
    float lightType;
    float lightTurnOn;
    float2 dummy;
    float3 ReflectDirection;
    float dummy2;
};

#include "commonPart.hlsl"

uint gTextureID : register(b0);
Texture2D gTechTextures[10] : register(t0);
SamplerState gSampler : register(s0);

[RootSignature(rootSignatureBlur)]
VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;
   
    vout.UVText = vin.UVText;    
    // Already in homogeneous clip space.
    vout.PosH = float4(vin.PosL, 1.0f);    
		
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return float4(gTechTextures[gTextureID].Sample(gSampler, pin.UVText).xyz, 1.0f);
}