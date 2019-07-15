
#include "Lighting.hlsl"
#include "SkinnedCommonPart.hlsl"
#include "RootSignature.hlsl"

[RootSignature(rootSignatureC1)]
VertexOut VS(VertexIn vin, uint instID : SV_INSTANCEID)
{
	VertexOut vout;

    uint shapeID = gDrawInstancesIDData[instID + gInstancesOffset];
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
    float4x4 lFinalM = mul(Final, wordMatrix);

    float4 posW = mul(lFinalM, float4(vin.PosL, 1.0f));   
	vout.PosW = posW.xyz;

    float4x4 ViewProj = cbPass.ViewProj;

    // Transform to homogeneous clip space.    
    vout.PosH = mul(posW, ViewProj);   
    
    vout.NormalW = mul((float3x3) wordMatrix, vin.Normal );
    float3 tangentNU = vin.TangentU;//   normalize(vin.TangentU - dot(vin.TangentU, vin.Normal));
    vout.TangentW = float4(mul((float3x3) wordMatrix, tangentNU), 0.0f);
    
    vout.UVText = vin.UVText;

    //// for projection
    //matrix<float, 4, 4> T;
    //T._11_12_13_14 = float4(0.5f, 0.0f, 0.0f, 0.0f);
    //T._21_22_23_24 = float4(0.0f, -0.5f, 0.0f, 0.0f);
    //T._31_32_33_34 = float4(0.0f, 0.0f, 1.0f, 0.0f);
    //T._41_42_43_44 = float4(0.5f, 0.5f, 0.0f, 1.0f);

    //vout.UVTextProj = mul(vout.PosH, T);

    //float4 orthogProj = mul(posW, cbPass.MirWord);
    //float4 SSAOProj = mul(posW, cbPass.ViewProj); 
    //vout.UVTextProj = mul(orthogProj, T);
    //vout.SSAOPosH = mul(SSAOProj, T);
    vout.SSAOPosH = mul(posW, cbPass.ViewProjT);

    vout.ShapeID = shapeID; 
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{	
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
      
    
    // Get SSAO factor
    float ssao_factor = 1.0f;
    if ((gTechFlags & (1 << RTB_SSAO)) > 0) // if we use SSAO information
    {
        float4 lUV = pin.SSAOPosH ;// mul(float4(pin.PosW, 1.0f), cbPass.ViewProjT);
        lUV.xyz /= lUV.w;
        ssao_factor = gSSAOBlurMap.Sample(gsamPointWrap, lUV.xy).r;
    }    
    //return float4(ssao_factor, ssao_factor, ssao_factor, 1.0f);    
    float4 ambient = ssao_factor * cbPass.AmbientLight * diffuseAlbedo;
            
    const float shiness = 0.0f;//  1.0f - material.Roughness;
    MaterialLight matLight = { diffuseAlbedo, material.FresnelR0, shiness };
    
    float shadow_depth;// = CalcShadowFactor(pin.UVTextProj, gDiffuseMap[0], gsamShadow);
    
    shadow_depth = 1.0f;
    float4 directLight = ComputeLighting(cbPass.Lights, matLight, pin.PosW, Normal, toEyeW, shadow_depth);
        
    float4 litColor = directLight + ambient;
    
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

