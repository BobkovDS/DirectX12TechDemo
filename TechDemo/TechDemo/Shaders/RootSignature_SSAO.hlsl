#define rootSignatureSSAO "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),\
CBV(b0),\
DescriptorTable( SRV(t0, numDescriptors = 3), visibility=SHADER_VISIBILITY_PIXEL),\
StaticSampler(s0, addressU=TEXTURE_ADDRESS_CLAMP, addressV=TEXTURE_ADDRESS_CLAMP, addressW=TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_POINT, visibility = SHADER_VISIBILITY_PIXEL),\
StaticSampler(s1, maxAnisotropy =0, addressU=TEXTURE_ADDRESS_BORDER, addressV=TEXTURE_ADDRESS_BORDER, addressW=TEXTURE_ADDRESS_BORDER, filter = FILTER_MIN_MAG_MIP_LINEAR, borderColor=STATIC_BORDER_COLOR_OPAQUE_WHITE,  visibility = SHADER_VISIBILITY_PIXEL),\
StaticSampler(s2, addressU=TEXTURE_ADDRESS_WRAP, addressV=TEXTURE_ADDRESS_WRAP, addressW=TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR, visibility = SHADER_VISIBILITY_PIXEL)"
 

ConstantBuffer<PassStructSSAO> cbPass : register(b0);
Texture2D gRandomVectorMap : register(t0);
Texture2D gViewNormalMap : register(t1);
Texture2D gDepthMap : register(t2);
SamplerState gsamPointClamp : register(s0);
SamplerState gsamDepthMap : register(s1);
SamplerState gsamLinerWrap : register(s2);