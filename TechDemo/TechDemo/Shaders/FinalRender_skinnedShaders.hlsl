
#include "Lighting.hlsl"
#include "SkinnedCommonPart.hlsl"
#include "RootSignature.hlsl"

//int instID : SV_INSTANCEID

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

   // wordMatrix = mul(Final, wordMatrix);
    
    //get World transform
    //float4 posW = mul(float4(vin.PosL, 1.0f), wordMatrix);
    float4 posW = mul(wordMatrix, float4(vin.PosL, 1.0f) );
   // float4 posW = float4(vin.PosL, 1.0f);
    posW = mul(Final, posW);
	vout.PosW = posW.xyz;

    float4x4 ViewProj = cbPass.ViewProj;

    // Transform to homogeneous clip space.    
    vout.PosH = mul(posW, ViewProj);
    
   
    
    vout.NormalW = mul(vin.Normal, (float3x3) wordMatrix);
    float3 tangentNU = vin.TangentU;//   normalize(vin.TangentU - dot(vin.TangentU, vin.Normal));
    vout.TangentW = float4(mul(tangentNU, (float3x3) wordMatrix), 0.0f);
    
    vout.UVText = vin.UVText;

    // for projection
    matrix<float, 4, 4> T;
    T._11_12_13_14 = float4(0.5f, 0.0f, 0.0f, 0.0f);
    T._21_22_23_24 = float4(0.0f, -0.5f, 0.0f, 0.0f);
    T._31_32_33_34 = float4(0.0f, 0.0f, 1.0f, 0.0f);
    T._41_42_43_44 = float4(0.5f, 0.5f, 0.0f, 1.0f);

   // vout.UVTextProj = mul(vout.PosH, T);

    float4 orthogProj = mul(posW, cbPass.MirWord);
    float4 SSAOProj = mul(posW, cbPass.ViewProj);
   //if (gShadowUsed)
   //     orthogProj = mul(posW, cbPass.MirWord); //ShadowWord MirWord
   // else
   //     orthogProj = mul(posW, cbPass.ViewProj);
    vout.UVTextProj = mul(orthogProj, T);
    vout.SSAOPosH = mul(SSAOProj, T);
    vout.ShapeID = shapeID;
   //vout.instID = instID;
    
    return vout;
}

struct PixelOut
{
    float4 color : SV_Target;
    float depth : SV_Depth;
};

float4 PS(VertexOut pin) : SV_Target
//PixelOut PS(VertexOut pin)
{
    //if (gShadowUsed) return float4(0.05f ,0.0f ,0.0f ,1.0f); //For Stencil exercises 
   // return float4(0.5f ,0.0f ,0.0f ,1.0f); //For Stencil exercises 
	
    pin.NormalW = normalize(pin.NormalW);
    float3 toEyeW = cbPass.EyePosW - pin.PosW;
    float distToEye = length(toEyeW);
    toEyeW = toEyeW / distToEye;
    
    float spotFactor = 1.0f;

    InstanceData instData = gInstanceData[pin.ShapeID];
    MaterialData material = gMaterialData[instData.MaterialIndex];
    
    float4 diffuseAlbedo = material.DiffuseAlbedo;
    float diffuseTranspFactor = 0.0f;

    float3 Normal = pin.NormalW;

    if ((material.textureFlags & 0x01))
        diffuseAlbedo = gDiffuseMap[material.DiffuseMapIndex[0]].Sample(gsamPointWrap, pin.UVText);    
    
    if ((material.textureFlags & 0x10))
    {
        //diffuseAlbedo = gDiffuseMap[material.DiffuseMapIndex[4]].Sample(gsamPointWrap, pin.UVText);
        //diffuseTranspFactor = diffuseAlbedo.x;
        diffuseTranspFactor = gDiffuseMap[material.DiffuseMapIndex[4]].Sample(gsamPointWrap, pin.UVText);
        
    }
     
      
    
    //if ((material.textureFlags & 0x02) && gShadowUsed && 0) //&& gShadowUsed
    //{
    //   float4 readNormal = gDiffuseMap[material.DiffuseMapIndex[1]].Sample(gsamPointWrap, pin.UVText);
    //   Normal = NormalSampleToWorldSpace(readNormal.xyz, Normal, pin.TangentW);
    //}

    //float2 ssaouv = pin.SSAOPosH.xy / pin.SSAOPosH.w;
    //float ssao_factor = gDiffuseMap[3].Sample(gsamPointWrap, ssaouv).r;  
    float ssao_factor = 1.0f;
    float4 ambient = ssao_factor * cbPass.AmbientLight * diffuseAlbedo;
            
    const float shiness = 0.0f;//  1.0f - material.Roughness;
    MaterialLight matLight = { diffuseAlbedo, material.FresnelR0, shiness };
    
    float shadow_depth;// = CalcShadowFactor(pin.UVTextProj, gDiffuseMap[0], gsamShadow);
    
    shadow_depth = 1.0f;
    float4 directLight = ComputeLighting(cbPass.Lights, matLight, pin.PosW, Normal, toEyeW, shadow_depth);

    float4 litColor = directLight;//  +ambient;
    litColor = diffuseAlbedo; //  +ambient;
    
    //if (cbPass.FogRange > 0)
    if (0> 1)
    {
        float fogAmount = saturate((distToEye - cbPass.FogStart) / cbPass.FogRange);
        litColor = lerp(litColor, cbPass.FogColor, fogAmount);
    }    

    // get light from cube map reflection
    //float3 r = reflect(-toEyeW, Normal);
    //float4 reflectionColor = gCubeMap.Sample(gsamPointWrap, r);
    //float3 fresnelFactor = SchlickFresnel(material.FresnelR0, Normal, r);    
    
    //litColor.rgb += (1.0f - material.Roughness) * fresnelFactor * reflectionColor.rgb;       
    litColor.a = diffuseTranspFactor; //   diffuseAlbedo.a;
	return litColor;
}

