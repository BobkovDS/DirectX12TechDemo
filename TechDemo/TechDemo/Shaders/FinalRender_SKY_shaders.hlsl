
#include "Lighting.hlsl"
#include "commonPart.hlsl"
#include "RootSignature.hlsl"


//int instID : SV_INSTANCEID

[RootSignature(rootSignatureC1)]
VertexOut VS(VertexIn vin, uint instID : SV_INSTANCEID)
{   
	VertexOut vout;

    ///uint shapeID = gDrawInstancesIDData[instID + gInstancesOffset];    
    uint shapeID = instID + gInstancesOffset;
    InstanceData instData = gInstanceData[shapeID];
    float4x4 wordMatrix = instData.World;  
    
    float4 posW = mul(wordMatrix, float4(vin.PosL, 1.0f));
    //vout.PosW = vin.PosL; // for texturing old
    vout.PosW = posW.xyz; // for texturing
    //get World transform
    
    
    //float4 posW = float4(vin.PosL, 1.0f);
    posW.xyz  += cbPass.EyePosW;

    // Transform to homogeneous clip space.    
    vout.PosH = mul(posW, cbPass.ViewProj).xyww; // is used with psoDescLayer5.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{   
    //return float4(pin.PosW, 1.0f);

   float4 lColor = gCubeMap.Sample(gsamPointWrap, pin.PosW);
   // float4 lColor = gDCMCubeMap.Sample(gsamPointWrap, pin.PosW);
    lColor.a = 1.0f;
    return lColor;
}

