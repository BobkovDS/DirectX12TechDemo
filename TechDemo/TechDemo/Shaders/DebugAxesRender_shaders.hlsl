
#include "Lighting.hlsl"
#include "commonPart.hlsl"
#include "DebugRootSignature.hlsl"

[RootSignature(rootSignatureDebug)]
VertexOut VS(VertexIn vin)
{   
	VertexOut vout;       
   
    float4 posW = float4(vin.PosL, 1.0f);    
    float4x4 ViewProj = cbPass.ViewProj;

    // Transform to homogeneous clip space.    
    vout.PosH = mul(posW, ViewProj);     
    vout.NormalW = vin.Normal; // here we store color
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{   	
    return float4(pin.NormalW, 1.0f); // we use NormalW for color
}

