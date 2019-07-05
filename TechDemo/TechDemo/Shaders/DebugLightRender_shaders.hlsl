
#include "Lighting.hlsl"
#include "commonPart.hlsl"
#include "RootSignature_Debug.hlsl"

[RootSignature(rootSignatureDebug)]
GeometryIn VS(VertexIn vin)
{
    //, uint vid : SV_VertexID
    GeometryIn v;
    v.instID = 0;
    return v;
}

[maxvertexcount(4)]
void GS(point GeometryIn inputVertex[1], inout LineStream<GeometryOut> outputLines)
{
    uint lLigthID = 0; //   inputVertex.instID;
    GeometryOut p1;
    GeometryOut p2;
    GeometryOut p3;
    
    float3 lightPosition = cbPass.Lights[lLigthID].Position;
    float3 lightDirection = cbPass.Lights[lLigthID].Direction;

    float factor = 20;
    float factorL = 2;
    p1.PosH = mul(float4(lightPosition, 1.0f), cbPass.ViewProj);
    p2.PosH = mul(float4(lightPosition + factor * lightDirection, 1.0f), cbPass.ViewProj);
    p3.PosH = mul(float4(lightPosition + factorL * lightDirection, 1.0f), cbPass.ViewProj);

    p1.TangentW = float4(0, 0, 0, 0);
    p1.PosW = float3(0, 0, 0);
    p1.UVText = float2(0, 0);    
    p1.ShapeID = 0;

    p2.TangentW = float4(0, 0, 0, 0);
    p2.PosW = float3(0, 0, 0);
    p2.UVText = float2(0, 0);
    p2.ShapeID = 0;

    p3.TangentW = float4(0, 0, 0, 0);
    p3.PosW = float3(0, 0, 0);
    p3.UVText = float2(0, 0);
    p3.ShapeID = 0;
   
    p3.NormalW = float3(0, 0, 0);
    p1.NormalW = p3.NormalW;   
    outputLines.Append(p1);
    outputLines.Append(p3);    
    outputLines.RestartStrip();

    p3.NormalW = cbPass.Lights[lLigthID].Strength;
    p2.NormalW = p3.NormalW;
    outputLines.Append(p2);
    outputLines.Append(p3);
    outputLines.RestartStrip();
};

float4 PS(GeometryOut pin) : SV_Target
{
    return float4(pin.NormalW, 1.0f); // we use NormalW for color    
}
