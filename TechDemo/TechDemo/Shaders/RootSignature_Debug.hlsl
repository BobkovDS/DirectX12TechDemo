#define rootSignatureDebug "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),\
 RootConstants(num32BitConstants=2, b0),\
 SRV(t0, space=1),\
 CBV(b3)"

uint gInstancesOffset:register(b0);
uint gTechFlags:register(b1);
ConstantBuffer<PassStruct> cbPass : register(b3);
StructuredBuffer<InstanceData> gInstanceData: register(t0, space1);