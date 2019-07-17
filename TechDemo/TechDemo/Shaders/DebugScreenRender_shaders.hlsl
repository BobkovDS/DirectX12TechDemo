#define rootSignatureScreen "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),\
RootConstants(num32BitConstants=1, b0),\
DescriptorTable( SRV(t0, numDescriptors = 6), UAV(u0, numDescriptors = 1), SRV(t6, numDescriptors = 1), UAV(u1, numDescriptors = 1), SRV(t7, numDescriptors = 1), visibility=SHADER_VISIBILITY_ALL),\
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
TextureCube gSkyCubeTexture: register(t0);
Texture2D gTechTextures[5] : register(t1);
Texture2D gTechTextures9 : register(t7);

SamplerState gSampler : register(s0);

[RootSignature(rootSignatureScreen)]
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
    if (gTextureID <=5)
        return float4(gTechTextures[gTextureID - 1].Sample(gSampler, pin.UVText).xyz, 1.0f); // -1 because we have also 
    else
        return float4(gTechTextures9.Sample(gSampler, pin.UVText).xyz, 1.0f); // -1 because we have also 
}