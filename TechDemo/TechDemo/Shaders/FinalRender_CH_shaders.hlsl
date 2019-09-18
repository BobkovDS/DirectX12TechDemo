
#include "Lighting.hlsl"
#include "commonPart.hlsl"
#include "RootSignature.hlsl"

[RootSignature(rootSignatureC1)]
VertexOut VS(VertexIn vin, uint instID : SV_InstanceID)
{    
    VertexOut vout;
    
    uint shapeID = instID + gInstancesOffset;
    InstanceData instData = gInstanceData[shapeID];
    MaterialData material = gMaterialData[instData.MaterialIndex];
    float4x4 wordMatrix = instData.World;
    
    /* 
        we use texture to get Height value for this Vertex.
    */
        
    float4 ltextValue = gDiffuseMap[material.DiffuseMapIndex[0]].SampleLevel(gsamPointWrap, vin.UVText, 0);
    
    //get World transform    
    vin.PosL.z = ltextValue.x;
    float4 posW = mul(wordMatrix, float4(vin.PosL, 1.0f));
   
    vout.PosW = posW.xyz;

    float4x4 ViewProj = cbPass.ViewProj;

    // Transform to homogeneous clip space.    
    vout.PosH = mul(posW, ViewProj);
    vout.NormalW = mul((float3x3) wordMatrix, ltextValue.yzw);
    vout.UVText = vin.UVText;    

    float3 tangentNU = vin.TangentU; 
    vout.TangentW = float4(mul((float3x3) wordMatrix, tangentNU), 0.0f);      
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
    
    InstanceData instData = gInstanceData[pin.ShapeID];
    MaterialData material = gMaterialData[instData.MaterialIndex];
    
    float4 diffuseAlbedo = material.DiffuseAlbedo;
    
    float3 Normal = pin.NormalW;
    float diffuseTranspFactor = 1.0f;
        
    diffuseTranspFactor = diffuseAlbedo.a;
       
    // Get Normal
    if (gTechFlags & 0x01 > 0)
    {
        if ((material.textureFlags & 0x04))
        {
            float4 readNormal = gDiffuseMap[material.DiffuseMapIndex[2]].Sample(gsamPointWrap, pin.UVText);
            Normal = NormalSampleToWorldSpace(readNormal.xyz, Normal, pin.TangentW);
        }
    }

    // Get Alpha value
    if ((material.textureFlags & 0x10))        
        diffuseTranspFactor = gDiffuseMap[material.DiffuseMapIndex[4]].Sample(gsamPointWrap, pin.UVText).x;
    
    // Get SSAO factor
    float ssao_factor = 1.0f;
    if ((gTechFlags & (1 << RTB_SSAO)) > 0) // if we use SSAO information
    {
        float2 lUV = pin.SSAOPosH.xy / pin.SSAOPosH.w;
        ssao_factor = gSSAOBlurMap.Sample(gsamPointWrap, lUV).r;
    }
        
    float4 ambient = ssao_factor * cbPass.AmbientLight * diffuseAlbedo;
    
    const float shiness = 1.0f;//  -material.Roughness;
    
    material.FresnelR0 = float3(0.08f, 0.08f, 0.08f);
    MaterialLight matLight = { diffuseAlbedo, material.FresnelR0, shiness };
        
    float shadow_depth = 1.0f;
    if ((gTechFlags & (1 << RTB_SHADOWMAPPING)) > 0)  // if we use Shadow mapping
    {
        float4 lShadowPosH = mul(float4(pin.PosW, 1.0f), cbPass.Lights[0].ViewProjT);
        shadow_depth = CalcShadowFactor(lShadowPosH, gShadowMap0, gsamShadow);
    }   

    //shadow_depth = 1.0f;
    float4 directLight = ComputeLighting(cbPass.Lights, matLight, pin.PosW, Normal, toEyeW, shadow_depth);

    float4 litColor = directLight + ambient;
    
    //if (cbPass.FogRange > 0)
    if (0 > 1)
    {
        float fogAmount = saturate((distToEye - cbPass.FogStart) / cbPass.FogRange);
        litColor = lerp(litColor, cbPass.FogColor, fogAmount);
    }        
       
    float3 r = reflect(-toEyeW, Normal);
    //float3 r = refract(-toEyeW, pin.NormalW, 0.9f);
    float4 reflectionColor = gCubeMap.Sample(gsamPointWrap, r);
    float3 fresnelFactor = SchlickFresnel(material.FresnelR0, Normal, r);
    //litColor.rgb += shiness * fresnelFactor * reflectionColor.rgb;
    litColor.rgb = reflectionColor.rgb;

    litColor.a = 0.5f;
    return litColor;
}