#include "Lighting.hlsl"
Texture2D tDiffuseMap : register(t0);
Texture2D tDiffuseMap1 : register(t1);
Texture2D tDiffuseMap2 : register(t2);

SamplerState sWrapPoint : register(s0);
SamplerState sWrapLinear : register(s1);

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
	float2 dummies;	
	Light gLights[MaxLights];
};

cbuffer cbMirror : register(b3)
{
   int gMirrorFlag;
};


struct VertexIn
{
	float3 PosL  : POSITION;
    float3 NormalL : NORMAL;
	float3 UVCoord : UVCOORD;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float3 PosW : POSITION;
    float3 NormalW : NORMAL;
	float2 TextC:TEXTCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut) 0.0f;
	
	float4x4 gUpdatedWorld;

	if (gMirrorFlag >0) 
	{
		gUpdatedWorld = ReflectWord;
	}
	else
	{
		gUpdatedWorld = gWorld;
	}
	
	// Get World transformation
	float4 posW = mul(float4(vin.PosL, 1.0f), gUpdatedWorld);
	vout.PosW = posW.xyz;

	// Transform to homogeneous clip space. 
	vout.PosH = mul(posW, gViewProj);
    
	//Assumes non-uniform scaling; otherwise, need to use inverse-transpose of world matrix;
	vout.NormalW = mul(vin.NormalL, (float3x3) gUpdatedWorld);
	
	vout.TextC = vin.UVCoord;
	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    
	pin.NormalW = normalize(pin.NormalW);
	
	float3 toEyeW = gEyePosW - pin.PosW;//normalize(gEyePosW - pin.PosW);	
	float distToEye = length(toEyeW);
	toEyeW = toEyeW/distToEye; //normalize
	
	//Indirect ligthing
		
	//pin.TextC = float2(0.5f, 0.5f);
	float4 diffuseAlbedo = gDiffuseAlbedo;
	if (gFlags & 1) 
			diffuseAlbedo = tDiffuseMap.Sample(sWrapLinear, pin.TextC);//*gDiffuseAlbedo;
	
	#ifdef APLHA_TEST
		clip(diffuseAlbedo.a - 0.1f);	
	#endif
	
	if (gFlags & 16) // only for Land surface
	{	
		float4 diffuseAlbedo2 = tDiffuseMap2.Sample(sWrapLinear, pin.TextC);
		
		pin.TextC.x = pin.TextC.x + 0.01f * cos(radians (gTotalTime ) );
		pin.TextC.y = pin.TextC.y + 0.01f * cos(radians (gTotalTime ));
		
		float4 diffuseMask = tDiffuseMap1.Sample(sWrapLinear, pin.TextC);
		diffuseAlbedo =  ((1.0f,1.0f,1.0f,1.0f) - diffuseMask) * diffuseAlbedo + diffuseMask * diffuseAlbedo2;
		
	}
	
	float4 ambient = gAmbientLight * diffuseAlbedo;
	
	//Direct Lighting
	const float shininess = 1.0f - gRoughness;
	Material  mat = { diffuseAlbedo, gFresnelR0, shininess };
	float4 directLight = ComputeLighting(gLights, mat, pin.PosW, pin.NormalW, toEyeW);
	
	float4 litColor =  ambient + directLight;
	
	#if defined(FOG)
		float fogAmount = saturate((distToEye-gFogStart)/gFogRange);
		litColor = lerp(litColor, gFogColor, fogAmount);	
	#endif
	
	litColor.a = diffuseAlbedo.a;    
    
    return litColor;
}


float4 PS_wireframe(VertexOut pin) : SV_Target
{
    float4 litColor = float4(1.0f, 0.2f, 0.2f, 1.0f);

    return litColor;
}