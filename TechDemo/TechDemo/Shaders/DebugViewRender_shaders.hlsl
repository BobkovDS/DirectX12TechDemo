/*
    To Draw Camera View Direction
*/

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
    
    float3 viewPointPosition = cbPass.ViewPointPosition;
    float3 begin = float3(0, -5.0f , 0);
    float3 Direction = float3(0, 1.0f, 0.0f);

    float factor = 200;    
    
    p1.TangentW = float4(0, 0, 0, 0);
    p1.PosW = float3(0, 0, 0);
    p1.UVText = float2(0, 0);
    p1.ShapeID = 0;

    p2 = p3 = p1;

    p1.PosH = mul(float4(viewPointPosition - factor * Direction, 1.0f), cbPass.ViewProj);
    p3.PosH = mul(float4(viewPointPosition, 1.0f), cbPass.ViewProj);
    p2.PosH = mul(float4(viewPointPosition + factor * Direction, 1.0f), cbPass.ViewProj);
   
    p1.NormalW = float3(0, 0, 0);
    p3.NormalW = p1.NormalW;   
    outputLines.Append(p1);
    outputLines.Append(p3);    
    outputLines.RestartStrip();

    p2.NormalW = float3(1.0f, 0, 0);
    p3.NormalW = p2.NormalW;
    outputLines.Append(p2);
    outputLines.Append(p3);
    outputLines.RestartStrip();

};

float4 PS(GeometryOut pin) : SV_Target
{
    return float4(pin.NormalW, 1.0f); // we use NormalW for color    
}
