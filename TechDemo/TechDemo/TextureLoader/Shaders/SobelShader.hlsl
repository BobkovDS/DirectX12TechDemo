
Texture2D tBase : register(t0);
Texture2D tSobel : register(t1);

SamplerState sClampPoint : register(s0);
SamplerState sWrapLinear : register(s1);

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float3 UVCoord : UVCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TextC : TEXTCOORD;
};

static const float2 gTextCoord[6] =
{
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f)
};

VertexOut VS(VertexIn vi, uint vid: SV_VertexID)
{
    VertexOut vout = (VertexOut) 0.0f;
        
    vout.TextC = gTextCoord[vid];
    vout.PosH = float4(2.0f * vout.TextC.x - 1.0f, 1.0f - 2.0f * vout.TextC.y, 0.0f, 1.0f);
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 c = tBase.SampleLevel(sClampPoint, pin.TextC, 0.0f);
    
    float4 e = tSobel.SampleLevel(sClampPoint, pin.TextC, 0.0f);
    
    return (float4(0.0f, 0.0f, 0.0f, 0.0f )+ c * e);
   // return e;
}


