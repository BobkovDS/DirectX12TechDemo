#define MaxLights 10

struct MaterialLight
{
	float4 DiffuseAlbedo;
	float3 FresnelR0;
	float Shininess;
};

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
    float4x4 ViewProj;
    float4x4 ViewProjT;
	float3 ReflectDirection;
	float dummy2;
};

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
	float result = saturate((falloffEnd - d) / (falloffEnd - falloffStart));

	return result;		
}

float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
	float cosIncidentAngle = saturate(dot(normal, lightVec));
	
	float f0 = 1.0f - cosIncidentAngle;
	float3 reflectPercent = R0 + (1.0f - R0)*(f0*f0*f0*f0*f0);

	return reflectPercent;
}


float3 BlinnPhong(float3 lightStrenght, float3 lightVec, float3 normal, float3 toEye, MaterialLight mat)
{
	const float m = mat.Shininess * 256.0f;
	float3 halfVec = normalize(toEye + lightVec);

	float roughnessFactor = (m + 8.0f)* pow(max(dot(halfVec, normal), 0), m) / 8.0f;
	float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);

	float3 specAlbedo = roughnessFactor * fresnelFactor;

	specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrenght;
     

}

// Direction Light
float3 ComputeDirectionalLight(Light L, MaterialLight mat, float3 normal, float3 toEye)
{
	//float3 lightVec = -L.ReflectDirection;
    float3 lightVec = -L.Direction;
	
	float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;
	
	return BlinnPhong(lightStrength,lightVec, normal, toEye, mat);
}


// Point Light
float3 ComputePointLight(Light L, MaterialLight mat, float3 pos, float3 normal, float3 toEye)
{
	float3 lightVec = L.Position - pos;

	float d = length(lightVec);
	if (d > L.FalloffEnd) return 0.0f;
	
	lightVec /= d;

	float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

	float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
	lightStrength *= att;

	return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

// Spot Light

float3 ComputeSpotLight(Light L, MaterialLight mat, float3 pos, float3 normal, float3 toEye)
{
	// The vector from the surface to the light
	float3 lightVec = L.Position - pos; 

	//The distance from surface to light
	float d = length (lightVec);
	
	if (d > L.FalloffEnd) return 0.0f;
	
	//normalize the light vector
	lightVec /=d;
	
	float ndotl = max(dot(lightVec, normal), 0.0f);	
	float3 lightStrength = L.Strength * ndotl;
	
	//Attenuate light by distance
	float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
	lightStrength *= att;
	
	//float spotFactor = pow(max(dot(-lightVec, L.ReflectDirection), 0.0f), L.SpotPower);
	L.Direction = L.Direction / length(L.Direction);
	float ndotl2 =  max(dot(-lightVec, L.Direction), 0.0f);
	float spotFactor = pow(ndotl2, L.SpotPower);
		
	lightStrength *= spotFactor;
	
	return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}


// Common Light
float4 ComputeLighting(Light glights[MaxLights], MaterialLight mat, float3 pos, float3 normal, float3 toEye, float shadowFactor)
{
	float3 result = 0.0f;
	//float3 result = float3(0.5f, 0.5f, 0.7f);
	for (int i=0; i<MaxLights; i++)
	{
		if (glights[i].lightTurnOn == 1)
		{
			if (glights[i].lightType == 1) // directional
			{                
				result += ComputeDirectionalLight(glights[i], mat, normal, toEye);
                if (i==0)
                {
                    result *= shadowFactor;
                }                
            }
			else if (glights[i].lightType == 2) // spot
			{
				result += ComputeSpotLight(glights[i], mat, pos, normal, toEye);
			} 
			else if (glights[i].lightType == 3) // point
			{
				result += ComputePointLight(glights[i], mat, pos, normal, toEye);
			}			
		}
	}
	return float4(result, 0.0f);	
}

float CalcShadowFactor(float4 shadowPosH, Texture2D shadowMap, SamplerComparisonState gsamShadow)
{
    //shadowPosH.xyz /= shadowPosH.w; //does not make sense for ortographic projection, 
    float depth = shadowPosH.z; 
    
    uint width, height, numMips;
    shadowMap.GetDimensions(0,width, height, numMips);
    float dx = 1.0f / (float) width;
    float dy = dx; //   1.0f / (float) height;
    float percentLit = 0.0f;

    // it is for a spot cone light
    //float x_factor = 0.5f - abs(shadowPosH.x - 0.5f);
    //float y_factor = 0.5f - abs(shadowPosH.y - 0.5f);        
    //float spot_factor = sqrt(x_factor * x_factor + y_factor * y_factor);
    //if (spot_factor > 1.0f) spot_factor = 1.0f;

    const float2 offset[9] =
    {
        float2(-dx, -dy), float2(0, -dy), float2(dx, -dy),
        float2(-dx, 0), float2(0, 0), float2(dx, 0),
        float2(-dx, dy), float2(0, dy), float2(dx, dy)
    };

    for (int i = 0; i < 9; ++i)
    {
        percentLit += shadowMap.SampleCmpLevelZero(gsamShadow, shadowPosH.xy + offset[i], depth).r;
        //percentLit += shadowMap.SampleCmp(gsamShadow, shadowPosH.xy + offset[i], depth).r;
    }   

    return (percentLit / 9.0f); // * spot_factor
}