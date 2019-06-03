#define MaxLights 10

struct Material
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
};

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
	return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
	float cosIncidentAngle = saturate(dot(normal, lightVec));
	
	float f0 = 1.0f - cosIncidentAngle;
	float3 reflectPercent = R0 + (1.0f - R0)*(f0*f0*f0*f0*f0);

	return reflectPercent;
}


float3 BlinnPhong(float3 lightStrenght, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
	const float m = mat.Shininess * 256.0f;
	float3 halfVec = normalize(toEye + lightVec);

	float roughnessFactor = (m + 8.0f)* pow(max(dot(halfVec, normal), 0), m) / 8.0f;
	float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);

	float3 specAlbedo = roughnessFactor * fresnelFactor;

	specAlbedo = specAlbedo / (specAlbedo + 1.0f);

	return (mat.DiffuseAlbedo.rgb + specAlbedo)*lightStrenght;
}

// Direction Light
float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye)
{
	float3 lightVec = -L.Direction;
	
	float ndotl = max(dot(lightVec, normal), 0.0f);
	float3 lightStrength = L.Strength * ndotl;
	
	return BlinnPhong(lightStrength,lightVec, normal, toEye, mat);
}


// Point Light
float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
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

float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
	// The vector from the surface to the light
	float3 lightVec = L.Position - pos; 
	//float3 lightVec = -L.Direction;
	
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
	
	float spotFactor = pow(max(dot(-lightVec, L.Direction), 0.0f), L.SpotPower);
	lightStrength *= spotFactor;
	
	return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}


// Common Light
float4 ComputeLighting(Light glights[MaxLights], Material mat, float3 pos, float3 normal, float3 toEye)
{
	float3 result = 0.0f;
	for (int i=0; i<MaxLights; i++)
	{
		if (glights[i].lightTurnOn == 1)
		{
			if (glights[i].lightType == 1)
			{
				result += ComputeDirectionalLight(glights[i], mat, normal, toEye);
			}
			else if (glights[i].lightType == 2)
			{
				result += ComputePointLight(glights[i], mat, pos, normal, toEye);
			} 
			else if (glights[i].lightType == 3)
			{
				result += ComputeSpotLight(glights[i], mat, pos, normal, toEye);
			} 
		}
	}
	return float4(result, 0.0f);	
}