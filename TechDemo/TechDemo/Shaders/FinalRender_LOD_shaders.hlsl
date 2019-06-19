
#include "Lighting.hlsl"
#include "commonPart.hlsl"
#include "RootSignature.hlsl"


//int instID : SV_INSTANCEID

[RootSignature(rootSignatureC1)]
GeometryOut VS(VertexIn vin, uint instID : SV_INSTANCEID) //
{
    GeometryOut vout;
    
    uint shapeID = instID + gInstancesOffset; // we do not use vin.ShapeID anymore in this variant    
    InstanceData instData = gInstanceData[shapeID];
    float4x4 wordMatrix = instData.World;
    
     //get World transform    
    float4 posW = mul(wordMatrix, float4(vin.PosL, 1.0f));
    float4x4 ViewProj = cbPass.ViewProj;
        
    vout.PosW = posW.xyz;
    vout.PosH = mul(posW, ViewProj);
    vout.NormalW = mul(vin.Normal, (float3x3) wordMatrix);
    vout.UVText = vin.UVText;
    vout.ShapeID = shapeID;
    
    return vout;
}

[maxvertexcount(3)]
void GS(triangle GeometryOut inPolygon[3], uint primID : SV_PrimitiveID, inout TriangleStream<GeometryOut> outPolygon)
{      
    uint lPolyginCountID = primID%2; //   primID % k_factor;

    if (lPolyginCountID == 0)
    { 
        outPolygon.Append(inPolygon[0]);
        outPolygon.Append(inPolygon[1]);
        outPolygon.Append(inPolygon[2]);
        outPolygon.RestartStrip();
    }
}

float4 PS(GeometryOut pin) : SV_Target
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
    float diffuseTranspFactor = 0.0f;

    if ((material.textureFlags & 0x01))
        diffuseAlbedo = gDiffuseMap[material.DiffuseMapIndex[0]].Sample(gsamPointWrap, pin.UVText);    
    
    diffuseTranspFactor = 1.0f- diffuseAlbedo.a;

    if ((material.textureFlags & 0x10))        
        diffuseTranspFactor = gDiffuseMap[material.DiffuseMapIndex[4]].Sample(gsamPointWrap, pin.UVText);        
    

    const float shiness = 0.0f;//  1.0f - material.Roughness;
    MaterialLight matLight = { diffuseAlbedo, material.FresnelR0, shiness };
        
    float shadow_depth = 1.0f;
    float4 directLight = ComputeLighting(cbPass.Lights, matLight, pin.PosW, Normal, toEyeW, shadow_depth);

    float4 litColor = directLight;//  +ambient;
    litColor = diffuseAlbedo; //  +ambient;
    
    //if (cbPass.FogRange > 0)
    if (0 > 1)
    {
        float fogAmount = saturate((distToEye - cbPass.FogStart) / cbPass.FogRange);
        litColor = lerp(litColor, cbPass.FogColor, fogAmount);
    }    
        
    litColor.a = diffuseTranspFactor;
	return litColor;
}