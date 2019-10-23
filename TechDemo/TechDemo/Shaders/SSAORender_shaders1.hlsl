
#include "Lighting.hlsl"
#include "commonPart.hlsl"
#include "RootSignature.hlsl"

[RootSignature(rootSignatureC1)]
VertexOut VS(VertexIn vin, uint instID : SV_INSTANCEID) 
{   
    VertexOut vout;

    uint shapeID = instID + gInstancesOffset; // we do not use vin.ShapeID anymore in this variant
    InstanceData instData = gInstanceData[shapeID];
    float4x4 wordMatrix = instData.World;
    
    //get World transform    
    float4 posW = mul(wordMatrix, float4(vin.PosL, 1.0f));    
    vout.PosW = posW.xyz;

    float4x4 ViewProj = cbPass.ViewProj;

    // Transform to homogeneous clip space.    
    vout.PosH = mul(posW, ViewProj);
    vout.NormalW = mul((float3x3) wordMatrix, vin.Normal);      
    vout.UVText = vin.UVText;

    float3 tangentNU = vin.TangentU;
    vout.TangentW = float4(mul((float3x3) wordMatrix, tangentNU), 0.0f);

    vout.ShapeID = shapeID;
    
    return vout;
}


float4 PS(VertexOut pin) : SV_Target
{    
    pin.NormalW = normalize(pin.NormalW);
    float3 lNormalV = mul(pin.NormalW, (float3x3) cbPass.View);
    return float4(lNormalV, 0.0f);    
}

