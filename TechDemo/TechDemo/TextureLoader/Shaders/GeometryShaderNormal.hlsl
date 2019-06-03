#include "Lighting.hlsl"

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
};

cbuffer cbMaterial : register(b1)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
    int gFlags;
}
cbuffer cbPass : register(b2)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float4x4 ReflectWord;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;
    float4 gFogColor;
    float gFogStart;
    float gFogRange;
    float CameraDistance;
    float dummies;
    Light gLights[MaxLights];
};


struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float3 UVCoord : UVCOORD;
};

struct VertexOut
{    
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 TextC : TEXTCOORD;  
    uint VertexID : VERTEXID;
};


//--------------------------------------------------- Vertex Shader --------------------------------------------
//--------------------------------------------------------------------------------------------------------------

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;
    vout.Pos = vin.PosL;
    vout.Normal = vin.NormalL;
    vout.TextC = vin.NormalL.xy;
    return vout;
}

//--------------------------------------------------- Geometry Shader ------------------------------------------
//--------------------------------------------------------------------------------------------------------------

struct GeometryOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TextC : TEXTCOORD;
    uint primID : PRIMITIVEID;
};
void OutPutSubdivision(VertexOut Vert, inout LineStream<GeometryOut> TrianglStreamOutput)
{
    GeometryOut gout;

    float4 tempPos = mul(float4(Vert.Pos, 1.0f), gWorld);
        
    gout.PosH = mul(tempPos, gViewProj);
    gout.PosW = tempPos.xyz;
    gout.NormalW = mul(Vert.Normal, (float3x3) gWorld);        
    gout.TextC = Vert.TextC;        
    gout.primID = 1;
  
    TrianglStreamOutput.Append(gout);
}
//-------------------------------------------- Geometry Shader to Draw Vertex Normal
[maxvertexcount(2)]
void GP(
	point VertexOut SHinput[1],  
    //uint primID: SV_PrimitiveID,
	inout LineStream<GeometryOut> output)
{    
    VertexOut V1;
    
    V1 = SHinput[0];
    V1.Pos = SHinput[0].Pos + 3 * SHinput[0].Normal;
    
    OutPutSubdivision(SHinput[0], output);
    OutPutSubdivision(V1, output);
}

//-------------------------------------------- Geometry Shader to Draw face Normal
[maxvertexcount(2)]
void GP_FN( //geometry shader _ Face Normal
	triangle VertexOut SHinput[3],
    //uint primID: SV_PrimitiveID,
	inout LineStream<GeometryOut> output)
{
    VertexOut Centr;
    Centr = SHinput[0];
    //SHinput[0].Pos = float3(0, -0.016, -0.450);
    //SHinput[1].Pos = float3(1.15, 0.850, 1.05);
    //SHinput[2].Pos = float3(-1.05, 1.716, -0.450);

    float deltaZero = 0.000001;
    float3 N0 = 0.5 * (SHinput[1].Pos + SHinput[2].Pos);
    float3 N1 = 0.5 * (SHinput[0].Pos + SHinput[1].Pos);
    
    float3 part1 = SHinput[2].Pos - SHinput[0].Pos;
    float3 V1 = N0 - SHinput[0].Pos;
    float3 V2 = N1 - SHinput[2].Pos;

    float d1 = dot(V1, V1);
    float d2 = dot(V1, V2);
    float d3 = dot(V2, V2);

    float detA = d1 * d3 - d2 * d2;
    
    if ((detA > deltaZero) || (detA < deltaZero))
    {
    
        float p2_1 = part1.x * V1.x + part1.y * V1.y + part1.z * V1.z;
        float p2_2 = -1 * (part1.x * V2.x + part1.y * V2.y + part1.z * V2.z);
        detA = 1 / detA;
        
        float t1 = detA * (p2_1*d3 + p2_2*d2);
        float t2 = detA * (p2_1 * d2 + p2_2 * d1);

        float3 centr = SHinput[0].Pos + V1 * t1;

        Centr.Pos = centr;

    }
    else
    {
        // Inverse Matrix does not exist. Need to think something
    }
  
    VertexOut NormalEnd;
    
    float3 Normal;
    Normal = cross(SHinput[1].Pos - SHinput[0].Pos, SHinput[2].Pos - SHinput[0].Pos);
    Normal = normalize(Normal);

    NormalEnd = SHinput[0];
    NormalEnd.Pos = Centr.Pos + 3 * Normal;

    OutPutSubdivision(Centr, output);
    OutPutSubdivision(NormalEnd, output);
   

}


//--------------------------------------------------- Pixel Shader ------------------------------------------
//--------------------------------------------------------------------------------------------------------------

float4 PS_FN(GeometryOut pin) : SV_Target
{	
    float4 litColor = float4(0.0f, 1.0f, 0.0f, 1.0f);

    return litColor;
}

float4 PS(GeometryOut pin) : SV_Target
{
    float4 litColor = float4(0.0f, 0.0f, 1.0f, 1.0f);

    return litColor;
}


