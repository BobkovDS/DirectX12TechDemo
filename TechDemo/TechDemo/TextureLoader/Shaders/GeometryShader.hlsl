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

struct Triangl
{
    VertexOut V[3];    
};

void SubdivideV2(VertexOut inVertices[3], out Triangl newTriangls[4])
{
    VertexOut newVertices[3];

    newVertices[0].Pos = 0.5f * (inVertices[0].Pos + inVertices[1].Pos);
    newVertices[1].Pos = 0.5f * (inVertices[1].Pos + inVertices[2].Pos);
    newVertices[2].Pos = 0.5f * (inVertices[2].Pos + inVertices[0].Pos);

    	//Derive normals
    
    newVertices[0].Normal = normalize(newVertices[0].Pos);
    newVertices[1].Normal = normalize(newVertices[1].Pos);
    newVertices[2].Normal = normalize(newVertices[2].Pos);
    
	// Project onto unit sphere
    newVertices[0].Pos = newVertices[0].Pos + normalize(newVertices[0].Pos);
    newVertices[1].Pos = newVertices[1].Pos + normalize(newVertices[1].Pos);
    newVertices[2].Pos = newVertices[2].Pos + normalize(newVertices[2].Pos);

    /*
	//Derive normals
    newVertices[0].Normal = newVertices[0].Pos;
    newVertices[1].Normal = newVertices[1].Pos;
    newVertices[2].Normal = newVertices[2].Pos;
 */    

	//Interpolate texture coordinates
    newVertices[0].TextC = 0.5f * (inVertices[0].TextC + inVertices[1].TextC);
    newVertices[1].TextC = 0.5f * (inVertices[1].TextC + inVertices[2].TextC);
    newVertices[2].TextC = 0.5f * (inVertices[2].TextC + inVertices[0].TextC);

    newTriangls[0].V[0] = inVertices[0];
    newTriangls[0].V[1] = newVertices[0];
    newTriangls[0].V[2] = newVertices[2];

    newTriangls[1].V[0] = newVertices[0];
    newTriangls[1].V[1] = inVertices[1];
    newTriangls[1].V[2] = newVertices[1];

    newTriangls[2].V[0] = newVertices[1];
    newTriangls[2].V[1] = inVertices[2];
    newTriangls[2].V[2] = newVertices[2];

    newTriangls[3].V[0] = newVertices[0];
    newTriangls[3].V[1] = newVertices[1];
    newTriangls[3].V[2] = newVertices[2];
}
void OutPutSubdivisionV2(Triangl triangl, uint PrimID, inout TriangleStream<GeometryOut> TrianglStreamOutput)
{
   GeometryOut gout[3];
   [unroll]
    for (int i = 0; i < 3; ++i)
    {
        float4 tempPos = mul(float4(triangl.V[i].Pos, 1.0f), gWorld);
        
        gout[i].PosH = mul(tempPos, gViewProj);

        gout[i].NormalW = mul(triangl.V[i].Normal, (float3x3) gWorld);
        
        gout[i].TextC = triangl.V[i].TextC;
        
        gout[i].primID = PrimID;
    }
    
    TrianglStreamOutput.Append(gout[0]);
    TrianglStreamOutput.Append(gout[1]);
    TrianglStreamOutput.Append(gout[2]);
    TrianglStreamOutput.RestartStrip();
    

}
void Animate(VertexOut input[3], out VertexOut output[3])
{
    float3 V0;
    float3 V1;
    float3 Normal;

    V0 = input[1].Pos - input[0].Pos;
    V1 = input[2].Pos - input[0].Pos;
    
    Normal = cross(V0, V1);

    output = input;

    output[0].Pos = input[0].Pos + gTotalTime * Normal;    
    output[1].Pos = input[1].Pos + gTotalTime * Normal;
    output[2].Pos = input[2].Pos + gTotalTime * Normal;

}

void RecursDevision(VertexOut inVertices[3], uint PrimID, inout TriangleStream<GeometryOut> TrianglStreamOutput)
{
    // Only for 3 Level Devision

    int RecursCount = 1;
    int TrianglCountOutput = 0;
    int TrianglCountInput = 0;
    int AllTrianglesCount = 0;
    VertexOut input[3] = inVertices;
    Triangl AllTriangles[64];
    Triangl DivideOutput[4];
    Triangl DivideInput[16];

    DivideInput[0].V = inVertices;
    AllTriangles[0].V = inVertices;
    TrianglCountInput = 1;
    AllTrianglesCount = 1; // it is for RecursCount==0

    if (CameraDistance > 100) RecursCount=0;
    else if ((CameraDistance > 60) && (CameraDistance <= 80)) RecursCount=1;
    else if ((CameraDistance > 10) && (CameraDistance <= 60)) RecursCount = 2;
    
    for (int i = 0;  i < RecursCount;  ++i)
    {
        AllTrianglesCount = 0;
        for (int tr = 0; tr < TrianglCountInput; ++tr)
        {
            SubdivideV2(DivideInput[tr].V, DivideOutput);
            for (int k = 0; k < 4; ++k)
            {
                AllTriangles[AllTrianglesCount] = DivideOutput[k];
                AllTrianglesCount++;
            }
        }
            
        for (int j = 0; j < AllTrianglesCount; ++j)
        {
            DivideInput[j] = AllTriangles[j];
        }
        TrianglCountInput = AllTrianglesCount;
    }

    GeometryOut gout;

    for (int t = 0; t < AllTrianglesCount;++t)
    {
        Triangl tmpTriangl;
        Animate(AllTriangles[t].V, tmpTriangl.V);

        OutPutSubdivisionV2(tmpTriangl, PrimID, TrianglStreamOutput);
    }  

}


[maxvertexcount(64)]
void myGeometryShader(
	triangle VertexOut SHinput[3],  
    //uint primID: SV_PrimitiveID,
	inout TriangleStream<GeometryOut> output)
{    
    VertexOut newForm[6];

    //Subdivide(SHinput, newForm);
    //OutPutSubdivision(newForm, primID, output);
    VertexOut animateOut[3];
    //Animate(SHinput, animateOut);
    RecursDevision(SHinput, 1, output);
}


//--------------------------------------------------- Pixel Shader ------------------------------------------
//--------------------------------------------------------------------------------------------------------------

float4 PS(GeometryOut pin) : SV_Target
{
    
    pin.NormalW = normalize(pin.NormalW);
	
    float3 toEyeW = gEyePosW - pin.PosW; //normalize(gEyePosW - pin.PosW);	
    float distToEye = length(toEyeW);
    toEyeW = toEyeW / distToEye; //normalize
	
	//Indirect ligthing
		
	//pin.TextC = float2(0.5f, 0.5f);
    float4 diffuseAlbedo = gDiffuseAlbedo;
    //if (gFlags & 1) diffuseAlbedo = tDiffuseMap.Sample(sWrapLinear, pin.TextC); //*gDiffuseAlbedo;

	/*
    if (gFlags & 16) // only for Land surface
    {
        float4 diffuseAlbedo2 = tDiffuseMap2.Sample(sWrapLinear, pin.TextC);
		
        pin.TextC.x = pin.TextC.x + 0.01f * cos(radians(gTotalTime));
        pin.TextC.y = pin.TextC.y + 0.01f * cos(radians(gTotalTime));
		
        float4 diffuseMask = tDiffuseMap1.Sample(sWrapLinear, pin.TextC);
        diffuseAlbedo = ((1.0f, 1.0f, 1.0f, 1.0f) - diffuseMask) * diffuseAlbedo + diffuseMask * diffuseAlbedo2;
		
    }
    */
	
    float4 ambient = gAmbientLight * diffuseAlbedo;
	
	//Direct Lighting
    const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW, pin.NormalW, toEyeW);
	
    float4 litColor = ambient + directLight;
	
#if defined(FOG)
		float fogAmount = saturate((distToEye-gFogStart)/gFogRange);
		litColor = lerp(litColor, gFogColor, fogAmount);	
#endif
	
    litColor.a = diffuseAlbedo.a;
    
    return litColor;
}