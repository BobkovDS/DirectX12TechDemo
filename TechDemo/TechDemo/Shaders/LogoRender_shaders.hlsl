
#include "Lighting.hlsl"
#include "commonPart.hlsl"

#define rootSignatureC1 "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),\
 SRV(t0, space=1),\
 CBV(b0)"

StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
ConstantBuffer<PassStruct> cbPass : register(b0);

[RootSignature(rootSignatureC1)]
VertexOut VS(VertexIn vin) 
{   
    VertexOut vout ;
  
    InstanceData instData = gInstanceData[0];
    float4x4 wordMatrix = instData.World;
    
    //get World transform    
    float4 posW = mul(wordMatrix, float4(vin.PosL, 1.0f));
    vout.PosW = posW.xyz;

    float4x4 ViewProj = cbPass.ViewProj;

    // Transform to homogeneous clip space.    
    vout.PosH = mul(posW, ViewProj);
    vout.NormalW = mul((float3x3) wordMatrix, vin.Normal);
    vout.TangentW = float4(vin.TangentU, 1.0f);
    vout.UVText = float2(0, 0);    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{       
    return pin.TangentW;    
}