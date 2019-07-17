#define rootSignatureBlur "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),\
RootConstants(num32BitConstants=1, b0),\
DescriptorTable( SRV(t0, numDescriptors = 1), visibility=SHADER_VISIBILITY_PIXEL),\
CBV(b1),\
DescriptorTable( SRV(t1, numDescriptors = 2), visibility=SHADER_VISIBILITY_PIXEL),\
StaticSampler(s0, addressU=TEXTURE_ADDRESS_CLAMP, addressV=TEXTURE_ADDRESS_CLAMP, addressW=TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_POINT, visibility = SHADER_VISIBILITY_PIXEL),\
StaticSampler(s1, addressU=TEXTURE_ADDRESS_BORDER, addressV=TEXTURE_ADDRESS_BORDER, addressW=TEXTURE_ADDRESS_BORDER, filter = FILTER_MIN_MAG_MIP_LINEAR, borderColor=STATIC_BORDER_COLOR_OPAQUE_WHITE,  visibility = SHADER_VISIBILITY_PIXEL)"

#define MaxLights 10
struct Light
{
    float3 Strength;
    float FalloffStart;
    float3 Direction;
    float FalloffEnd;
    float3 Position;
    float SpotPower;
    float lightType;
    float lightTurnOn;
    float2 dummy;
    float3 ReflectDirection;
    float dummy2;
};

static const int gBlurRadius = 5;

#include "commonPart.hlsl"

uint gBlurFlags : register(b0);
ConstantBuffer<PassStructSSAO> cbPass : register(b1);
Texture2D gInputMap : register(t0);
Texture2D gViewNormalMap : register(t1);
Texture2D gDepthMap : register(t2);

SamplerState gsamPointClamp : register(s0);
SamplerState gsamDepthMap : register(s1);

[RootSignature(rootSignatureBlur)]
VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;
   
    vout.UVText = vin.UVText;
    
    // Already in homogeneous clip space.
    vout.PosH = float4(vin.PosL, 1.0f);    
	
    float4 ph = mul(vout.PosH, cbPass.InvProj);
    vout.PosW = ph.xyz / ph.w; // here PosW should be calls "PosV", because this coordinates in View space
		
    return vout;
}

float OcclusionFunction(float distZ)
{
    float occlusion = 0.0f;
    if (distZ > cbPass.SurfaceEpsilon)
    {
        float fadeLength = cbPass.OcclusionFadeEnd - cbPass.OcclusionFadeStart;
        occlusion = saturate((cbPass.OcclusionFadeEnd - distZ) / fadeLength);
    }

    return occlusion;
}

float NdcDepthToViewDepth(float z_ndc)
{
    float viewZ = cbPass.Proj[3][2] / (z_ndc - cbPass.Proj[2][2]);
    return viewZ;
}

float4 PS(VertexOut pin) : SV_Target
{
    bool lHorizontalBlur = gBlurFlags & (1 << 0);   

    float blurWeights[12] =
    {
        cbPass.BlurWeight[0].x, cbPass.BlurWeight[0].y, cbPass.BlurWeight[0].z, cbPass.BlurWeight[0].w,
        cbPass.BlurWeight[1].x, cbPass.BlurWeight[1].y, cbPass.BlurWeight[1].z, cbPass.BlurWeight[1].w,
        cbPass.BlurWeight[2].x, cbPass.BlurWeight[2].y, cbPass.BlurWeight[2].z, cbPass.BlurWeight[2].w
    };
    
    float2 texOffset;
    if (lHorizontalBlur)
    {
        texOffset = float2(cbPass.InvRenderTargetSize.x, 0.0f);
    }
    else
    {
        texOffset = float2(0.0, cbPass.InvRenderTargetSize.y);
    }

    float totalWeight = blurWeights[gBlurRadius];
    float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);    
   
    color = totalWeight * gInputMap.SampleLevel(gsamPointClamp, pin.UVText, 0.0);   
       
    float3 centerNormal = gViewNormalMap.SampleLevel(gsamPointClamp, pin.UVText, 0.0f).xyz;
    float centerDepth = gDepthMap.SampleLevel(gsamDepthMap, pin.UVText, 0.0f).r;
    centerDepth = NdcDepthToViewDepth(centerDepth);
           
    for (float i = -gBlurRadius; i <= gBlurRadius; i++)
    {
        if (i == 0)
            continue;

        float2 tex = pin.UVText + i * texOffset;

        float3 neighborNormal = gViewNormalMap.SampleLevel(gsamPointClamp, tex, 0.0f).xyz;
        float neighborDepth = gDepthMap.SampleLevel(gsamDepthMap, tex, 0.0f).r;
        neighborDepth = NdcDepthToViewDepth(neighborDepth);

        float dt = dot(neighborNormal, centerNormal);
        float ddpth = abs(neighborDepth - centerDepth);
        if (dt >= 0.8f && ddpth <= 0.2f)
        {
            float weight = blurWeights[i + gBlurRadius];            
            color += weight * gInputMap.SampleLevel(gsamPointClamp, tex, 0.0);
            totalWeight += weight;
        }
    }

    totalWeight += 0.00002f;
    
    color /=totalWeight;   
    
    return color;
}
