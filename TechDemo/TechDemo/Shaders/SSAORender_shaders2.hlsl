
#include "Lighting.hlsl"
#include "commonPart.hlsl"
#include "RootSignature_SSAO.hlsl"

static const int gSampleCount = 14;

[RootSignature(rootSignatureSSAO)]
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
    float3 n = gViewNormalMap.SampleLevel(gsamPointClamp, pin.UVText, 0.0f).xyz;
    float pz = gDepthMap.SampleLevel(gsamDepthMap, pin.UVText, 0.0f).r;
    pz = NdcDepthToViewDepth(pz);
    
    float3 randon_vector = 2.0f * gRandomVectorMap.Sample(gsamPointClamp, pin.UVText, 0.0f).rgb - 1.0f;
        
    float3 p = (pz / pin.PosW.z) * pin.PosW;

    float occlusionSum = 0.0f;

    for (int i = 0; i < gSampleCount; i++)
    {
        float3 offset = cbPass.OffsetVectors[i].xyz;
        offset = reflect(offset, randon_vector);
        
        float flip = sign(dot(offset, n));
        float3 q = p + flip * cbPass.OcclusionRadius * offset;

        float4 projQ = mul(float4(q, 1.0f), cbPass.ProjTex);
        projQ /= projQ.w;

        float rz = gDepthMap.SampleLevel(gsamDepthMap, projQ.xy, 0.0f).r;
        rz = NdcDepthToViewDepth(rz);
        float3 r = (rz / q.z) * q;

        float distZ = p.z - r.z;
        float dp = max(dot(n, normalize(r - p)), 0.0f);
        if (dp < 0.1f)
            dp = 0;
        float occlusion = dp * OcclusionFunction(distZ); //dp * 
        occlusionSum += occlusion;
    }

    occlusionSum /= gSampleCount;
    float access = 1.0f - occlusionSum;
    
    return saturate(pow(access, 6.0f));
}


