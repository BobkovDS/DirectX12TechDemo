
#include "Lighting.hlsl"
#include "commonPart.hlsl"
#include "RootSignature.hlsl"


//int instID : SV_INSTANCEID

[RootSignature(rootSignatureC1)]
VertexOut VS(VertexIn vin, uint instID : SV_INSTANCEID) 
{   
	VertexOut vout;

    uint shapeID = instID + gInstancesOffset; // we do not use vin.ShapeID anymore in this variant
    InstanceData instData = gInstanceData[shapeID];
    float4x4 wordMatrix = instData.World;  
    
    //get World transform    
    float4 posW = mul(wordMatrix, float4(vin.PosL, 1.0f));    
    //float4 posW = float4(vin.PosL, 1.0f);    
    //posW.z = posW.z * (-1.0f);
	vout.PosW = posW.xyz;    

    float4x4 ViewProj = cbPass.ViewProj;

    // Transform to homogeneous clip space.    
    vout.PosH = mul(posW, ViewProj);    
    vout.NormalW = mul((float3x3) wordMatrix, vin.Normal);
    vout.UVText = vin.UVText;

    float3 tangentNU = vin.TangentU;//   normalize(vin.TangentU - dot(vin.TangentU, vin.Normal));
    vout.TangentW = float4(mul((float3x3) wordMatrix, tangentNU), 0.0f);

   // // for projection
   // matrix<float, 4, 4> T;
   // T._11_12_13_14 = float4(0.5f, 0.0f, 0.0f, 0.0f);
   // T._21_22_23_24 = float4(0.0f, -0.5f, 0.0f, 0.0f);
   // T._31_32_33_34 = float4(0.0f, 0.0f, 1.0f, 0.0f);
   // T._41_42_43_44 = float4(0.5f, 0.5f, 0.0f, 1.0f);

   //// vout.UVTextProj = mul(vout.PosH, T);

   // float4 orthogProj = mul(posW, cbPass.MirWord);
   // float4 SSAOProj = mul(posW, cbPass.ViewProj);
   
   // vout.UVTextProj = mul(orthogProj, T);
   // vout.SSAOPosH = mul(SSAOProj, T);

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
    float3 toEyeW = cbPass.EyePosW - pin.PosW;
    float distToEye = length(toEyeW);
    toEyeW = toEyeW / distToEye;
    
    float spotFactor = 1.0f;

    InstanceData instData = gInstanceData[pin.ShapeID];
    MaterialData material = gMaterialData[instData.MaterialIndex];
    
    float4 diffuseAlbedo = material.DiffuseAlbedo;
    
    float3 Normal = pin.NormalW;
    float diffuseTranspFactor = 1.0f;

    // Diffuse Color
    if ((material.textureFlags & 0x01))
        diffuseAlbedo = gDiffuseMap[material.DiffuseMapIndex[0]].Sample(gsamPointWrap, pin.UVText);        

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
    
    float ssao_factor = 0.5f;
    float4 ambient = ssao_factor * cbPass.AmbientLight * diffuseAlbedo;

    const float shiness = 0.0f;//  1.0f - material.Roughness;
    MaterialLight matLight = { diffuseAlbedo, material.FresnelR0, shiness };
        
    float shadow_depth = 1.0f;
    // if we use Shadow mapping
    if ((gTechFlags & (1 << RTB_SHADOWMAPPING)) > 0)
    {
        float4 lShadowPosH = mul(float4(pin.PosW, 1.0f), cbPass.Lights[0].ViewProjT);        
        shadow_depth = CalcShadowFactor(lShadowPosH, gShadowMap0, gsamShadow);
    }
    
    //shadow_depth = 1.0f;
    float4 directLight = ComputeLighting(cbPass.Lights, matLight, pin.PosW, Normal, toEyeW, shadow_depth);

    float4 litColor = directLight +ambient;
   // litColor = diffuseAlbedo +ambient;
    
    //if (cbPass.FogRange > 0)
    if (0 > 1)
    {
        float fogAmount = saturate((distToEye - cbPass.FogStart) / cbPass.FogRange);
        litColor = lerp(litColor, cbPass.FogColor, fogAmount);
    }    
        
    litColor.a = diffuseTranspFactor;
	return litColor;
}

