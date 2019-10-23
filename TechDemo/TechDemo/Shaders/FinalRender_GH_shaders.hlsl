
#include "Lighting.hlsl"
#include "commonPart.hlsl"
#include "RootSignature.hlsl"

[RootSignature(rootSignatureC1)]
VertexIn VS(VertexIn vin)
{
    return vin;
}

PatchTess ConstantHS(InputPatch<VertexIn, 4> patch, uint patchID : SV_PrimitiveID)
{
    float tess = 4.0f;
    
    //this is here kist to have a example how to calculate a tess koef for distance
    {    
        //float3 m = patch[3].PosL - patch[0].PosL;
        //int lShapeID = gInstancesOffset + 0; // Now we use only one xxx_GH object, in other case we need to use SV_INSTANCEID in vertex shader
        //InstanceData instData = gInstanceData[lShapeID];

        //float4 posW = mul(float4(m, 1.0f), instData.World);
        //float3 toEyeW = cbPass.EyePosW - posW.xyz;
        //float distToEye = length(toEyeW);
    
        //float koef = 1 - distToEye / 10;        
        //if (distToEye > 10)
        //    tess = 2;
        //else
        //    tess = lerp(2, 62, koef);
    }
    PatchTess pt;
    
    tess = 60; // 
    pt.EdgeTess[0] = tess;
    pt.EdgeTess[1] = tess;
    pt.EdgeTess[2] = tess;
    pt.EdgeTess[3] = tess;

    pt.InsideTess[0] = tess;
    pt.InsideTess[1] = tess;
    
    return pt;
}

//[domain("quad")]
[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")] //triangle_cw
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
VertexIn HS(InputPatch<VertexIn, 4> p, uint i : SV_OutputControlPointID, uint patchID : SV_PrimitiveID)
{ // creates new control points
    VertexIn vout;
    vout = p[i];
    return vout;
}

float2 updateUVLow(float2 uv, float4x4 worldM)
{   
    //return uv;
    float2 result;
    float4 uv4 = float4(uv, 1.0f, 1.0f);
    matrix<float, 4, 4> textMat;
    textMat = transpose(worldM);
    textMat._43 = 0.01f * cbPass.TotalTime;
    result = mul(uv4, textMat).xz;
    result = uv;
    result.y += 0.01f * cbPass.TotalTime;
    
    return result;
}

float2 updateUVHigh(float2 uv, float4x4 worldM)
{   
    float2 result;
    float4 uv4 = float4(uv, 0.0f, 1.0f);
    matrix<float, 4, 4> textMat;
    textMat = worldM;
   // textMat._34 = 0.005f * cbPass.TotalTime;
    
    result = mul(textMat,uv4).xz;
    return result;
}

[domain("quad")]
[RootSignature(rootSignatureC1)]
GeometryOut DS(PatchTess patchTess, float2 uv : SV_DomainLocation, const OutputPatch<VertexIn, 4> inPatch)
{
    GeometryOut dout;

    float3 p1 = lerp(inPatch[0].PosL, inPatch[1].PosL, uv.x);
    float3 p2 = lerp(inPatch[2].PosL, inPatch[3].PosL, uv.x);
    float3 p3 = lerp(p1, p2, uv.y);
    
    float2 uv1 = lerp(inPatch[0].UVText, inPatch[1].UVText, uv.x);
    float2 uv2 = lerp(inPatch[2].UVText, inPatch[3].UVText, uv.x);
    float2 uv3 = lerp(uv1, uv2, uv.y);

    int lShapeID = gInstancesOffset + 0; // Now we only one xxx_GH object, in other case we need to use SV_INSTANCEID in vertex shader
    
    InstanceData instData = gInstanceData[lShapeID];
    float4x4 wordMatrix = instData.World;
    MaterialData material = gMaterialData[instData.MaterialIndex];
   
    float4 heigh_low = { 0.0f, 0.0f, 0.0f, 0.0f };
    float4 heigh_high = { 0.0f, 0.0f, 0.0f, 0.0f };
    
    float2 textC_low =
    updateUVLow(uv3, instData.World);
    float2 textC_high = updateUVHigh(uv3, instData.World);

    if (material.textureFlags && 0x01)
        heigh_low = gDiffuseMap[material.DiffuseMapIndex[0]].SampleLevel(gsamPointWrap, textC_low, 0);

    if (material.textureFlags & 0x02)   
        heigh_high = gDiffuseMap[material.DiffuseMapIndex[1]].SampleLevel(gsamPointWrap, textC_high, 0);
   
       
    float kf = 1.0f - heigh_low.w;
    float hn = lerp(0.0f, 0.05f, kf); 
    
    float hnh = heigh_high.w;    
    
    p3.z += hn * 0.08 + 0.02 * hnh;
    p3.z -= 0.1;
    p3.z = hn;    

     //get World transform    
    float4 posW = mul(wordMatrix, float4(p3, 1.0f));
    dout.PosW = posW.xyz;

     // Transform to homogeneous clip space.    
    dout.PosH = mul(posW, cbPass.ViewProj);
	
    //dout.NormalW = mul(newNormal, (float3x3) instData.World);
    dout.NormalW = inPatch[0].Normal;    
    dout.UVText = uv3;
    dout.ShapeID = lShapeID;
    return dout;
};

[maxvertexcount(3)]
void GS(triangle GeometryOut inputTrianglVertecis[3], inout TriangleStream<GeometryOut> outputLines)
{
    GeometryOut p1 = inputTrianglVertecis[0];
    GeometryOut p2 = inputTrianglVertecis[1];
    GeometryOut p3 = inputTrianglVertecis[2];

    float3 Normal;
    Normal = cross(p2.PosW - p1.PosW, p3.PosW - p1.PosW);
   // Normal = normalize(Normal);
    
    float4x4 worldM = gInstanceData[p1.ShapeID].World;

    Normal = -mul(worldM, float4(Normal, 1.0f)).xyz;
    Normal = normalize(Normal);
    
    p1.NormalW = Normal;
    p2.NormalW = Normal;
    p3.NormalW = Normal;
 
    float3 Q1 = p2.PosW - p1.PosW; // we use vertext in World space
    float3 Q2 = p3.PosW - p1.PosW;
    float2 st1 = p2.UVText - p1.UVText;
    float2 st2 = p3.UVText - p1.UVText;

    float r = 1 / (st1.x * st2.y - st2.x * st1.y);
    
    float3 T;
    T.x = (st2.y * Q1.x - st1.y * Q2.x) * r;
    T.y= (st2.y * Q1.y - st1.y * Q2.y) * r;
    T.z = (st2.y * Q1.z - st1.y * Q2.z) * r;

    float3 B;
    B.x = (st1.x * Q2.x - st2.x * Q1.x) * r;
    B.y = (st1.x * Q2.y - st2.x * Q1.y) * r;
    B.z = (st1.x * Q2.z - st2.x * Q1.z) * r;

    T = T;
    //calculate handedness
    float handedness = dot(cross(Normal, T), B.x);
    handedness = handedness < 0.0f ? -1.0f : 1.0f;
       
    p1.TangentW = float4(T, handedness);
    p2.TangentW = float4(T, handedness);
    p3.TangentW = float4(T, handedness);

    outputLines.Append(p1);
    outputLines.Append(p2);
    outputLines.Append(p3);
};

float4 PS(GeometryOut pin) : SV_Target
{ 
    pin.NormalW = normalize(pin.NormalW); 

    float3 toEyeW = cbPass.EyePosW - pin.PosW;
    float distToEye = length(toEyeW);
    toEyeW = toEyeW / distToEye;
    
    InstanceData instData = gInstanceData[pin.ShapeID];
    MaterialData material = gMaterialData[instData.MaterialIndex];
    
    float4 diffuseAlbedo = material.DiffuseAlbedo;
    diffuseAlbedo += float4(0.0f, 0.0f, -0.2f, -0.2f);

    float2 textC1 = updateUVLow(pin.UVText, instData.World);
    
    float2 textC2 = pin.UVText;
    
    float3 normal1 = pin.NormalW;
    float3 normal2 = pin.NormalW;
      
    if ((material.textureFlags & 0x01)) //&& gShadowUsed
    {
        float4 readNormal = gDiffuseMap[material.DiffuseMapIndex[0]].Sample(gsamPointWrap, textC1);    
        normal1 = NormalSampleToWorldSpace(readNormal.xyz, normal1, pin.TangentW);        
    }
       
    if ((material.textureFlags & 0x02)) //&& gShadowUsed
    {
        float4 readNormal = gDiffuseMap[material.DiffuseMapIndex[1]].Sample(gsamPointWrap, textC2);
        normal2 = NormalSampleToWorldSpace(readNormal.xyz, normal2, pin.TangentW);
    }
   
    //float3 bumpedNormalW = normalize(normal1 + normal2);
    float3 bumpedNormalW = normal1;//   normalize(normal1);    
	
    float4 ambient = cbPass.AmbientLight * diffuseAlbedo;

    const float shiness = 1.0f - material.Roughness;
    float shadow_depth = 1.0f;
    MaterialLight matLight = { diffuseAlbedo, material.FresnelR0, shiness };
    float4 directLight = ComputeLighting(cbPass.Lights, matLight, pin.PosW, bumpedNormalW, toEyeW, shadow_depth);
    
    float4 litColor = ambient + directLight;
    
    float3 r = reflect(-toEyeW, bumpedNormalW);
    float4 reflectionColor = gCubeMap.Sample(gsamPointWrap, r);
    float3 fresnelFactor = SchlickFresnel(material.FresnelR0, bumpedNormalW, r);
    litColor.rgb += shiness * fresnelFactor * reflectionColor.rgb;

    litColor.a = 0.5f; //   +abs(cos(cbPass.TotalTime));
    return litColor;
}
