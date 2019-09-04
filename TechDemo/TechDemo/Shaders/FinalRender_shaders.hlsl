
#include "Lighting.hlsl"
#include "commonPart.hlsl"
#include "RootSignature.hlsl"


//int instID : SV_INSTANCEID

[RootSignature(rootSignatureC1)]
VertexOut VS(VertexIn vin, uint instID : SV_INSTANCEID) 
{   
	VertexOut vout;
    
    uint shapeID = instID + gInstancesOffset; 
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

    float3 tangentNU = vin.TangentU;//   normalize(vin.TangentU - dot(vin.TangentU, vin.Normal));
    vout.TangentW = float4(mul((float3x3) wordMatrix, tangentNU), 0.0f);
   
   // vout.UVTextProj = mul(orthogProj, T);
    vout.SSAOPosH = mul(posW, cbPass.ViewProjT);
    vout.ShapeID = shapeID;   
    
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
    //return float4(1.0f, 0.0f, 0.0f, 0.0f);

    pin.NormalW = normalize(pin.NormalW);
    //return float4(pin.NormalW, 1.0f);

    float3 toEyeW = cbPass.EyePosW - pin.PosW;
    float distToEye = length(toEyeW);
    toEyeW = toEyeW / distToEye;
    
    InstanceData instData = gInstanceData[pin.ShapeID];
    MaterialData material = gMaterialData[instData.MaterialIndex];
    
    float4 diffuseAlbedo = material.DiffuseAlbedo;
    
    float3 Normal = pin.NormalW;
    float diffuseTranspFactor = 1.0f;
   
    // Diffuse Color
    if ((material.textureFlags & 0x01))
        diffuseAlbedo = gDiffuseMap[material.DiffuseMapIndex[0]].Sample(gsamPointWrap, pin.UVText);        

    //return diffuseAlbedo;
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
        diffuseTranspFactor = gDiffuseMap[material.DiffuseMapIndex[4]].Sample(gsamPointWrap, pin.UVText);        
    
    // Get SSAO factor
    float ssao_factor = 1.0f;
    if ((gTechFlags & (1 << RTB_SSAO)) > 0) // if we use SSAO information
    {
        float2 lUV = pin.SSAOPosH.xy / pin.SSAOPosH.w;        
        ssao_factor = gSSAOBlurMap.Sample(gsamPointWrap, lUV).r;
    }   
    
    //return float4(ssao_factor, ssao_factor, ssao_factor, 1.0f);
    float4 ambient = ssao_factor * cbPass.AmbientLight * diffuseAlbedo;
    
    const float shiness = 1.0f - material.Roughness;
    material.FresnelR0 = float3(0.02f, 0.02f, 0.02f);
    MaterialLight matLight = { diffuseAlbedo, material.FresnelR0, shiness };
        
    float shadow_depth = 1.0f;   
    if ((gTechFlags & (1 << RTB_SHADOWMAPPING)) > 0)  // if we use Shadow mapping
    {
        float4 lShadowPosH = mul(float4(pin.PosW, 1.0f), cbPass.Lights[0].ViewProjT);        
        shadow_depth = CalcShadowFactor(lShadowPosH, gShadowMap0, gsamShadow);
    }
   // return float4(shadow_depth, shadow_depth, shadow_depth, 1.0f);

    //shadow_depth = 1.0f;
    float4 directLight = ComputeLighting(cbPass.Lights, matLight, pin.PosW, Normal, toEyeW, shadow_depth);

    float4 litColor = directLight + ambient;
    
    //if (cbPass.FogRange > 0)
    if (0 > 1)
    {
        float fogAmount = saturate((distToEye - cbPass.FogStart) / cbPass.FogRange);
        litColor = lerp(litColor, cbPass.FogColor, fogAmount);
    }    
        
    
    float3 r = refract(-toEyeW, Normal, 0.9f);
    float4 reflectionColor = gCubeMap.Sample(gsamPointWrap, r);
    float3 fresnelFactor = SchlickFresnel(material.FresnelR0, Normal, r);
    litColor.rgb += shiness * fresnelFactor * reflectionColor.rgb;

    litColor.a = diffuseTranspFactor;
	return litColor;
}

