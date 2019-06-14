#define rootSignatureC1 "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),\
 RootConstants(num32BitConstants=2, b0),\
 SRV(t0, space=1),\
 SRV(t1, space=1),\
 SRV(t0, space=2),\
 CBV(b3),\
 DescriptorTable( SRV(t0, numDescriptors = 10), visibility=SHADER_VISIBILITY_ALL),\
 DescriptorTable( SRV(t10, numDescriptors = unbounded), visibility=SHADER_VISIBILITY_ALL),\
 StaticSampler(s0, visibility = SHADER_VISIBILITY_ALL),\
 StaticSampler(s1, addressU=TEXTURE_ADDRESS_BORDER, addressV=TEXTURE_ADDRESS_BORDER, borderColor=STATIC_BORDER_COLOR_OPAQUE_BLACK, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc =COMPARISON_LESS_EQUAL, visibility=SHADER_VISIBILITY_PIXEL)"
 
//StaticSampler(s1, filter = FILTER_ANISOTROPIC, comparisonFunc = COMPARISON_LESS_EQUAL, visibility = SHADER_VISIBILITY_ALL)"

uint gInstancesOffset:register(b0);
uint gTechFlags:register(b1);
StructuredBuffer<InstanceData> gInstanceData: register(t0, space1);
StructuredBuffer<MaterialData> gMaterialData: register(t1, space1);
StructuredBuffer<BoneData> cbBoneData : register(t0, space2);
ConstantBuffer<PassStruct> cbPass : register(b3);
TextureCube gCubeMap : register(t0);
//Texture2D gDiffuseMap3: register(t1);
Texture2D gDiffuseMap[20] : register(t10);

SamplerState gsamPointWrap : register(s0);
SamplerComparisonState gsamShadow : register(s1);


//StaticSampler(s0, addressU=TEXTURE_ADDRESS_BORDER, addressV=TEXTURE_ADDRESS_BORDER, visibility = SHADER_VISIBILITY_ALL )"
//StaticSampler(s0, visibility = SHADER_VISIBILITY_ALL )"
//borderColor=STATIC_BORDER_COLOR_OPAQUE_BLACK,

// StaticSampler(s1, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc =COMPARISON_FUNC_LESS, visibility = SHADER_VISIBILITY_ALL )"