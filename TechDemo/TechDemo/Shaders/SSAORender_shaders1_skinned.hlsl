
#include "Lighting.hlsl"
#include "SkinnedCommonPart.hlsl"
#include "RootSignature.hlsl"

[RootSignature(rootSignatureC1)]
VertexOut VS(VertexIn vin, uint instID : SV_INSTANCEID)
{
	VertexOut vout;

    uint shapeID = instID + gInstancesOffset; // we do not use vin.ShapeID anymore in this variant
    InstanceData instData = gInstanceData[shapeID];
    float4x4 wordMatrix = instData.World;
    
    int boneIds[16] =
    {
        vin.BoneIndices0.x, vin.BoneIndices0.y, vin.BoneIndices0.z, vin.BoneIndices0.w,
        vin.BoneIndices1.x, vin.BoneIndices1.y, vin.BoneIndices1.z, vin.BoneIndices1.w,
        vin.BoneIndices2.x, vin.BoneIndices2.y, vin.BoneIndices2.z, vin.BoneIndices2.w,
        vin.BoneIndices3.x, vin.BoneIndices3.y, vin.BoneIndices3.z, vin.BoneIndices3.w,
    };

    float boneWeights[16] =
    {
        vin.BoneWeight0.x, vin.BoneWeight0.y, vin.BoneWeight0.z, vin.BoneWeight0.w,
        vin.BoneWeight1.x, vin.BoneWeight1.y, vin.BoneWeight1.z, vin.BoneWeight1.w,
        vin.BoneWeight2.x, vin.BoneWeight2.y, vin.BoneWeight2.z, vin.BoneWeight2.w,
        vin.BoneWeight3.x, vin.BoneWeight3.y, vin.BoneWeight3.z, vin.BoneWeight3.w,
    };
       
    float4x4 Final;
    for (int i = 0; i < 16; i++)
    {
        int boneID = boneIds[i];
        float boneWeight = boneWeights[i];
        float4x4 boneTr = cbBoneData[boneID].Transform;
        
        Final += boneTr * boneWeight;
    }      

    float4x4 lFinalM = mul(Final, wordMatrix);

    vout.NormalW = mul((float3x3) lFinalM, vin.Normal);
    //vout.NormalW = mul(vout.NormalW, (float3x3) cbPass.View);
    //vout.NormalW = normalize(vout.NormalW);
    vout.TangentW = float4(mul((float3x3) lFinalM, vin.TangentU), 0.0f);
    vout.UVText = vin.UVText;

    //get World transform
    
    float4 posW = mul(lFinalM, float4(vin.PosL, 1.0f));   
	vout.PosW = posW.xyz;

    // Transform to homogeneous clip space.    
    vout.PosH = mul(posW, cbPass.ViewProj);
    vout.ShapeID = instID;
    return vout;
}

/*
    Use the same Pixel Shader with "SSAORender_shaders1.hlsl"
*/