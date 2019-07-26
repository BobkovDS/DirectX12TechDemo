
#include "Lighting.hlsl"
#include "commonPart.hlsl"
#include "RootSignature.hlsl"

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

    float4x4 ViewProj = cbPass.Lights[0].ViewProj;
   // ViewProj = cbPass.ViewProj;

    // Transform to homogeneous clip space.    
    vout.PosH = mul(posW, ViewProj);    
    vout.NormalW = float3(0.0f, 0.0f, 0.0f);
    vout.UVText = vin.UVText;
    vout.TangentW = float4(0.0f, 0.0f, 0.0f, 0.0f);

    vout.ShapeID = shapeID;    
    return vout;
}

void PS(VertexOut pin) 
{   	 
#ifdef ALPHA_TEST
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
#endif
}

