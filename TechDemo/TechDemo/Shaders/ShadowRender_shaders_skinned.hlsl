
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
    
    //get World transform
    float4 posW = mul(wordMatrix, float4(vin.PosL, 1.0f) );   
    posW = mul(Final, posW);    
	vout.PosW = posW.xyz;

    float4x4 ViewProj = cbPass.Lights[0].ViewProj;

    // Transform to homogeneous clip space.    
    vout.PosH = mul(posW, ViewProj);
    vout.UVText = vin.UVText;
    vout.NormalW = float3(0.0f, 0.0f, 0.0f);
    vout.TangentW = float4(0.0f, 0.0f, 0.0f, 0.0f);
  
    vout.ShapeID = shapeID;
    
    return vout;
}

void PS(VertexOut pin) 
{
    InstanceData instData = gInstanceData[pin.ShapeID];
    MaterialData material = gMaterialData[instData.MaterialIndex];
    
    float4 diffuseAlbedo = material.DiffuseAlbedo;
    
    float3 Normal = pin.NormalW;
    float diffuseTranspFactor = 1.0f;

    // Diffuse Color
    if ((material.textureFlags & 0x01))
        diffuseAlbedo = gDiffuseMap[material.DiffuseMapIndex[0]].Sample(gsamPointWrap, pin.UVText);

    diffuseTranspFactor = diffuseAlbedo.a;

    clip(diffuseTranspFactor - 0.1f);

}

