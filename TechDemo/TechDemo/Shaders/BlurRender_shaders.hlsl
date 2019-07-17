#define rootSignatureBlur "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),\
RootConstants(num32BitConstants=1, b0),\
DescriptorTable( SRV(t0, numDescriptors = 1), UAV(u0, numDescriptors = 1)),\
DescriptorTable( SRV(t2, numDescriptors = 2)),\
CBV(b1),\
StaticSampler(s0, addressU=TEXTURE_ADDRESS_CLAMP, addressV=TEXTURE_ADDRESS_CLAMP, addressW=TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_POINT),\
StaticSampler(s1, addressU=TEXTURE_ADDRESS_BORDER, addressV=TEXTURE_ADDRESS_BORDER, addressW=TEXTURE_ADDRESS_BORDER, filter = FILTER_MIN_MAG_MIP_LINEAR, borderColor=STATIC_BORDER_COLOR_OPAQUE_WHITE)"

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

uint gBlurFlags : register(b0);
Texture2D gInputMap : register(t0);
RWTexture2D<float4> gOutputMap : register(u0);
Texture2D gViewNormalMap : register(t2);
Texture2D gDepthMap : register(t3);
ConstantBuffer<PassStructSSAO> cbPass : register(b1);

SamplerState gsamPointClamp : register(s0);
SamplerState gsamDepthMap : register(s1);

static const int gBlurRadius = 5;
#define N 256
#define CacheSize (N + 2*5)
groupshared float4 gCache[CacheSize];
groupshared float3 gCacheNormal[CacheSize];
groupshared float gCacheDepth[CacheSize];

float NdcDepthToViewDepth(float z_ndc)
{
    float viewZ = cbPass.Proj[3][2] / (z_ndc - cbPass.Proj[2][2]);
    return viewZ;
}

[RootSignature(rootSignatureBlur)]
[numthreads(N, 1, 1)]
void CS_HOR(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{
    bool lHorizontalBlur = gBlurFlags & (1 << 0);
    /*
        When we do Vertical Blur, dispatchThreadID means:
            dispatchThreadID.x = Y for Textures
            dispatchThreadID.y = X for Textures
    */
    
    float blurWeights[12] =
    {
        cbPass.BlurWeight[0].x, cbPass.BlurWeight[0].y, cbPass.BlurWeight[0].z, cbPass.BlurWeight[0].w,
        cbPass.BlurWeight[1].x, cbPass.BlurWeight[1].y, cbPass.BlurWeight[1].z, cbPass.BlurWeight[1].w,
        cbPass.BlurWeight[2].x, cbPass.BlurWeight[2].y, cbPass.BlurWeight[2].z, cbPass.BlurWeight[2].w
    };
    uint lTextureSizeW;
    uint lTextureSizeH;
    gInputMap.GetDimensions(lTextureSizeW, lTextureSizeH);
    lTextureSizeW = lTextureSizeW - 1;
    lTextureSizeH = lTextureSizeH - 1;

   
    float2 uv;
    // Read Texel from Texture into cache
    if (groupThreadID.x < gBlurRadius)
    {
        int2 p;       
        if (lHorizontalBlur) // do Horizontal Blur
        {
            p.x = max(dispatchThreadID.x - gBlurRadius, 0);
            p.y = dispatchThreadID.y;            
        }
        else // do Vertical Blur 
        {
            p.x = dispatchThreadID.y;
            p.y = max(dispatchThreadID.x - gBlurRadius, 0);
        }
        
        gCache[groupThreadID.x] = gInputMap[p];

        uv.x = (p.x / (float) lTextureSizeW);
        uv.y = (p.y / (float) lTextureSizeH);
        
        gCacheNormal[groupThreadID.x] = gViewNormalMap.SampleLevel(gsamPointClamp, uv, 0.0f).xyz;
        gCacheDepth[groupThreadID.x] = gDepthMap.SampleLevel(gsamDepthMap, uv, 0.0f).r;
    }

    if (groupThreadID.x >= N - gBlurRadius)
    {
        int2 p;
        if (lHorizontalBlur) // do Horizontal Blur
        {
            p.x = min(dispatchThreadID.x + gBlurRadius, lTextureSizeW);
            p.y = dispatchThreadID.y;
        }
        else // do Vertical Blur
        {
            p.x = dispatchThreadID.y;
            p.y = min(dispatchThreadID.x + gBlurRadius, lTextureSizeH);
        }
        gCache[groupThreadID.x + 2 * gBlurRadius] = gInputMap[p];

        uv.x = (p.x / (float) lTextureSizeW);
        uv.y = (p.y / (float) lTextureSizeH);
        
        gCacheNormal[groupThreadID.x + 2 * gBlurRadius] = gViewNormalMap.SampleLevel(gsamPointClamp, uv, 0.0f).xyz;
        gCacheDepth[groupThreadID.x + 2 * gBlurRadius] = gDepthMap.SampleLevel(gsamDepthMap, uv, 0.0f).r;
    }

    int2 p;
    if (lHorizontalBlur)
    {
        p = min(dispatchThreadID.xy, uint2(lTextureSizeW, lTextureSizeH));
    }     
    else
    {
        p = min(dispatchThreadID.yx, uint2(lTextureSizeW, lTextureSizeH));
    }
        
    gCache[groupThreadID.x + gBlurRadius] = gInputMap[p];
 
    uv.x = (p.x / (float) lTextureSizeW);
    uv.y = (p.y / (float) lTextureSizeH);
    
    gCacheNormal[groupThreadID.x + gBlurRadius] = gViewNormalMap.SampleLevel(gsamPointClamp, uv, 0.0f).xyz;
    gCacheDepth[groupThreadID.x + gBlurRadius] = gDepthMap.SampleLevel(gsamDepthMap, uv, 0.0f).r;

    // Wait when all thread are done with reading theirs Texels
    GroupMemoryBarrierWithGroupSync();

    // Do Blur work
    
    float totalWeight = blurWeights[gBlurRadius];
    float4 blurColor = totalWeight * gCache[groupThreadID.x + gBlurRadius];   
    float3 centerNormal = gCacheNormal[groupThreadID.x + gBlurRadius];
    float centerDepth = gCacheDepth[groupThreadID.x + gBlurRadius];
    centerDepth = NdcDepthToViewDepth(centerDepth);

    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        if (i == 0)   continue;

        int k = groupThreadID.x + gBlurRadius + i;
        float3 neighborNormal = gCacheNormal[k];
        float neighborDepth = gCacheDepth[k];
        neighborDepth = NdcDepthToViewDepth(neighborDepth);

        float dt = dot(neighborNormal, centerNormal);
        float dt_dpth = abs(neighborDepth - centerDepth);
        if (dt >= 0.5 && dt_dpth <= 0.2f) 
        {
            totalWeight += blurWeights[i + gBlurRadius];
            blurColor += blurWeights[i + gBlurRadius] * gCache[k];
        }
    }

    totalWeight += 0.000001f;
    blurColor /= totalWeight;
    gOutputMap[p] = blurColor;
}