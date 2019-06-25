
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
    
    vout.PosW = vin.PosL; // for texturing
    //get World transform
    
    float4 posW = mul(wordMatrix, float4(vin.PosL, 1.0f));
    posW.xyz  += cbPass.EyePosW;

    // Transform to homogeneous clip space.    
    vout.PosH = mul(posW, cbPass.ViewProj).xyww; // is used with psoDescLayer5.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{   
    float4 lColor = gCubeMap.Sample(gsamPointWrap, pin.PosW);
    lColor.a = 0;
    return lColor;
}

