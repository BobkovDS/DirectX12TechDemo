
#include "Lighting.hlsl"
#include "commonPart.hlsl"
#include "DebugRootSignature.hlsl"

[RootSignature(rootSignatureDebug)]
GeometryIn VS(VertexIn vin, uint instID : SV_InstanceID)
{   
    GeometryIn vout;
    vout.PosL = vin.PosL;
    vout.Normal = normalize(vin.Normal);
    vout.UVText = vin.UVText;
    vout.instID = instID + gInstancesOffset;
    return vout;
}

void PutVertexOut2(GeometryIn vert, inout LineStream<GeometryOut> theOutput)
{
    GeometryOut outVertex;

    InstanceData instData = gInstanceData[vert.instID];
    float4 lPosW = mul(instData.World, float4(vert.PosL, 1.0f));
    outVertex.PosW = lPosW.xyz;
    outVertex.PosH = mul(lPosW, cbPass.ViewProj);
    outVertex.NormalW = float3(1.0f, 1.0f, 1.0f);
    outVertex.ShapeID = vert.instID;
    outVertex.TangentW = float4(0.0f, 0.0f, 0.0f, 0.0f);
    outVertex.UVText = float2(0.0f, 0.0f);

    theOutput.Append(outVertex);
}

[maxvertexcount(2)]
void GS_VN(point GeometryIn inputVertex[1], inout LineStream<GeometryOut> outputLines)
{
    float lOffset = 0.2;

    GeometryIn p1 = inputVertex[0];
    GeometryIn p2 = p1;
    p2.PosL = p1.PosL + lOffset * p1.Normal;

    PutVertexOut2(p1, outputLines);
    PutVertexOut2(p2, outputLines);
}

[maxvertexcount(2)]
void GS_FN(point GeometryIn inputVertex[1], inout LineStream<GeometryOut> outputLines)
{
    float lOffset = 1;

    GeometryIn p1 = inputVertex[0];
    GeometryIn p2 = p1;
    p2.PosL = p1.PosL + lOffset * p1.Normal;

    PutVertexOut2(p1, outputLines);
    PutVertexOut2(p2, outputLines);
}

float4 PS(GeometryOut pin) : SV_Target
{   	
    return float4(pin.NormalW, 1.0f); // we use NormalW for color
}

