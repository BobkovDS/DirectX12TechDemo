#define rootSignatureCompute1 "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),\
RootConstants(num32BitConstants=3, b0),\
DescriptorTable(UAV(u0, numDescriptors = 3)),\
CBV(b1),\
StaticSampler(s0, addressU=TEXTURE_ADDRESS_CLAMP, addressV=TEXTURE_ADDRESS_CLAMP, addressW=TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_POINT)"

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
    float2 pad;
    float k1;
    float k2;
    float k3;
};

uint gXPos: register(b0);
uint gYPos: register(b1);
uint gMagnitude: register(b2);

RWTexture2D<float4> gOutputBuffer : register(u0);
RWTexture2D<float4> gCurrentBuffer : register(u1);
RWTexture2D<float4> gPrevBuffer: register(u2);
ConstantBuffer<PassStructSSAO> cbPass : register(b1);
SamplerState gsamPointClamp : register(s0);

#define N 256
#define CacheSize N 
groupshared float4 gCache[CacheSize];

[RootSignature(rootSignatureCompute1)]
[numthreads(1, N, 1)]
void CS(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{
    /*
        The meaning of texture's components:
        gOutputBuffer.x - Vertex Height
        
        gCurrentBuffer.x - current Value (we use it do not read from texture one more time (from gPrevBuffer)
        gCurrentBuffer.y - previous Value
    */

    uint lTextureSizeW;
    uint lTextureSizeH;
    gOutputBuffer.GetDimensions(lTextureSizeW, lTextureSizeH);
    
    if (dispatchThreadID.y < 1 || dispatchThreadID.y > lTextureSizeH-1)
        return;    

    lTextureSizeW = lTextureSizeW ;
    //float k1 = cbPass.k1;
    //float k2 = cbPass.k2;
    //float k3 = cbPass.k3;

    float4 cV = gCurrentBuffer[int2(1, dispatchThreadID.y)];
    float4 lV = gCurrentBuffer[int2(0, dispatchThreadID.y)];

    for (int i = 1; i < lTextureSizeW-1; i++)
    {   
        float4 cV = gCurrentBuffer[int2(i, dispatchThreadID.y)];
        float4 lV = gCurrentBuffer[int2(i-1, dispatchThreadID.y )];
        float4 rV = gCurrentBuffer[int2(i + 1, dispatchThreadID.y)];
        float4 tV = gCurrentBuffer[int2(i, dispatchThreadID.y - 1)];
        float4 bV = gCurrentBuffer[int2(i, dispatchThreadID.y + 1)];

        float newHeight = cbPass.k1 * cV.x + cbPass.k2 * cV.y + cbPass.k3 * (rV.x + lV.x + tV.x + bV.x);
                
        gPrevBuffer[int2(i, dispatchThreadID.y)] = float4(newHeight, cV.x, cV.z, cV.w);
        
       // lV = cV;
       // cV = rV;
    }     

    // make a Drop
    
    float lMagnitude = gMagnitude / 100.0f;
    float lHalfMagnitude = lMagnitude / 2.0f;
    if ((gYPos == (dispatchThreadID.y - 1)) || gYPos == (dispatchThreadID.y + 1))
    {  
        gPrevBuffer[int2(gXPos - 1, gYPos)] = float4(lHalfMagnitude, 0.0f, lV.z, lV.w);
        gPrevBuffer[int2(gXPos, gYPos)] = float4(lHalfMagnitude, 0.0f, lV.z, lV.w);
        gPrevBuffer[int2(gXPos + 1, gYPos)] = float4(lHalfMagnitude, 0.0f, lV.z, lV.w);
    }
    
    if (gYPos == dispatchThreadID.y )
    {       
        gPrevBuffer[int2(gXPos - 1, gYPos)] = float4(lHalfMagnitude, 0.0f, lV.z, lV.w);
        gPrevBuffer[int2(gXPos, gYPos)] = float4(lMagnitude, 0.0f, lV.z, lV.w);
        gPrevBuffer[int2(gXPos + 1, gYPos)] = float4(lHalfMagnitude, 0.0f, lV.z, lV.w);
    }
  

     // Wait when all threads are done with calculation
    GroupMemoryBarrierWithGroupSync();

    // Calculate Normals and write down all results to Textures
    
    cV = gPrevBuffer[int2(1, dispatchThreadID.y)];
    lV = gPrevBuffer[int2(0, dispatchThreadID.y)];

    float dx = 0.078f;

    for (int ii = 1; ii < lTextureSizeW-1; ii++)
    {        
        float4 rV = gPrevBuffer[int2(ii + 1, dispatchThreadID.y)];
        float4 tV = gPrevBuffer[int2(ii, dispatchThreadID.y - 1)];
        float4 bV = gPrevBuffer[int2(ii, dispatchThreadID.y + 1)];

        float3 Normal = float3(lV.x - rV.x, 2.0f * dx, (tV.x - bV.x));
        
        float4 cachValue = cV;
        gCurrentBuffer[int2(ii, dispatchThreadID.y)] = cachValue;

        // updated with Normals or Tangent
        gOutputBuffer[int2(ii, dispatchThreadID.y)] = float4(cachValue.x, Normal);

        lV = cV;
        cV = rV;
    }
}