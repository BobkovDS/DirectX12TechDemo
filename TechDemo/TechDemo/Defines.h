#pragma once

// Scene object and PSO layers
#define SKY 0
#define OPAQUELAYER SKY + 1
#define NOTOPAQUELAYER OPAQUELAYER +1
#define SKINNEDOPAQUELAYER OPAQUELAYER +2
#define SKINNEDNOTOPAQUELAYER OPAQUELAYER +3
#define NOTOPAQUELAYERGH OPAQUELAYER +4
#define LAYERSCOUNT NOTOPAQUELAYERGH - SKY + 1

// InputLayout variants
#define ILV1 0
#define ILV2 ILV1 +1 
#define ILVCOUNT ILV2 - ILV1 + 1

// Draw flags for DebugRender_Normals
#define DRN_VERTEX 0 // To draw Vertex Normal and ID PSO for this
#define DRN_FACE 1 // To draw Face Normal and ID PSO for this
#define DRN_TANGENT 2 // To draw Tangent Normal and ID PSO for this
#define PSO_NORMALS_COUNT 2

// Render Mode
#define RM_FINAL 0
#define RM_SSAO_MAP1 1
#define RM_SSAO_MAP2 2
#define RM_SSAO_MAP3 3
#define RM_SSAO_MAPS ((1 << RM_SSAO_MAP1)| (1 << RM_SSAO_MAP2) | (1 << RM_SSAO_MAP3))
#define RM_CLEAR_ALL ~((1<<RM_FINAL) | (1<<RM_SSAO_MAP1)| (1<<RM_SSAO_MAP2) | (1<<RM_SSAO_MAP3))