
#define RTB_OFFSET 4
#define RTB_SSAO (RTB_OFFSET + 0)
#define RTB_SHADOWMAPPING (RTB_OFFSET + 1)
#define RTB_NORMALMAPPING (RTB_OFFSET + 2)

struct BoneData
{
    float4x4 Transform;
};

struct InstanceData
{
    float4x4 World;    
    uint MaterialIndex;
};

struct MaterialData
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Roughness;

    float4x4 MatTransform;

    uint textureFlags;
    uint DiffuseMapIndex[6];
     //for alignment
    uint Pad0;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 Normal : NORMAL;
	float4 TangentU : TANGENT;
    float2 UVText : TEXCOORD;    
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
	float4 TangentW : TANGENT;
    float2 UVText : TEXCOORD;
    float4 UVTextProj : TEXCOORDPROJ;
    float4 SSAOPosH : TEXCOORDPROJSSAO;
    uint ShapeID : SHAPEID;
};

struct GeometryIn// Vertex shader output - Geometry Shader input
{
    float3 PosL : POSITION;
    float3 Normal : NORMAL;    
    float2 UVText : TEXCOORD;    
    uint instID : SHAPEID;
};

struct GeometryOut
{
    float4 PosH : SV_POSITION;    
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;  
    float4 TangentW : TANGENT;
    float2 UVText : TEXCOORD;        	
    uint ShapeID : SHAPEID;
};

struct PassStruct
{
    float4x4 View;    
    float4x4 Proj;
    float4x4 InvProj;
    float4x4 ViewProj;
    float4x4 ViewProjT;
   
    float3 EyePosW; 
    float pad0;

    float4 AmbientLight;
    float4 FogColor;
    
    float FogStart;
    float FogRange;    
    float TotalTime;
    float DeltaTime;
    
    Light Lights[MaxLights];
};

struct PassStructSSAO
{
	float4x4 Proj;
    float4x4 InvProj;
	float4x4 ProjTex;
    float4 OffsetVectors[14];
    float4 BlurWeight[3];	   
    float2 InvRenderTargetSize;
    float OcclusionRadius;
    float OcclusionFadeStart;
    float OcclusionFadeEnd;
    float SurfaceEpsilon;    	
};

struct PatchTess
{
   float EdgeTess[4] : SV_TessFactor;
   float InsideTess[2] : SV_InsideTessFactor;   
};

float3 NormalSampleToWorldSpace(float3 normaleSample, float3 unitNormalW, float4 tangentW)
{
    float3 normalT = 2.0f * normaleSample - 1.0f;
    float3 N = unitNormalW;
    float3 T = normalize(tangentW.xyz - dot(tangentW.xyz, N) * N);
    float w = tangentW.w < 0.0f ? -1.0f : 1.0f;              

    float3 B = cross(N, T) * w;

    float3x3 TBN = float3x3(T, B, N);

    float3 bumpedNormaleW = mul(normalT, TBN);
    bumpedNormaleW = normalize(bumpedNormaleW);
    return bumpedNormaleW;
}