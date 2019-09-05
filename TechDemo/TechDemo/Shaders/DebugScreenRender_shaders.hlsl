#define rootSignatureScreen "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),\
RootConstants(num32BitConstants=1, b0),\
DescriptorTable( SRV(t0, numDescriptors = 7), UAV(u0, numDescriptors = 1), SRV(t7, numDescriptors = 1), UAV(u1, numDescriptors = 1), SRV(t8, numDescriptors = 1), visibility=SHADER_VISIBILITY_ALL),\
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
    float4x4 ViewProj;
    float4x4 ViewProjT;
    float3 ReflectDirection;
    float dummy2;
};

#include "commonPart.hlsl"

uint gTextureID : register(b0);
TextureCube gSkyCubeTexture: register(t0);
TextureCube gDCMCubeTexture: register(t1);
Texture2D gTechTextures[5] : register(t2);  
Texture2D gTechTextures10 : register(t8);

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
    if (gTextureID <=7)
        return float4(gTechTextures[gTextureID - 2].Sample(gSampler, pin.UVText).xyz, 1.0f); // -2 because we have also gSkyCubeTexture and gDCMCubeTexture
    else
        return float4(gTechTextures10.Sample(gSampler, pin.UVText).xyz, 1.0f); 
}