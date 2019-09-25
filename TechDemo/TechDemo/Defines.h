#pragma once

// Scene object and PSO layers
#define SKY 0
#define OPAQUELAYER SKY + 1
#define SKINNEDOPAQUELAYER OPAQUELAYER +1
#define SKINNEDNOTOPAQUELAYER OPAQUELAYER +2
#define NOTOPAQUELAYERGH OPAQUELAYER + 3
#define NOTOPAQUELAYERCH OPAQUELAYER + 4
#define NOTOPAQUELAYER OPAQUELAYER + 5
#define LAYERSCOUNT NOTOPAQUELAYER - SKY + 1

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
#define RM_SHADOW 4
#define RM_SSAO_MAPS ((1 << RM_SSAO_MAP1)| (1 << RM_SSAO_MAP2) | (1 << RM_SSAO_MAP3))
#define RM_OTHERMODE (RM_SSAO_MAPS | (1<<RM_SHADOW))
#define RM_CLEAR_ALL ~((1<<RM_FINAL) | (1<<RM_SSAO_MAP1)| (1<<RM_SSAO_MAP2) | (1<<RM_SSAO_MAP3))

//// Using Technics (UT)
//#define UT_SSAO 0 
//#define UT_SHADOW 1
//#define UT_NormalMapping 2
//#define UT_ALL ((1<<UT_SSAO) | (1<<UT_SHADOW) | (1<<UT_NormalMapping))

// Render selected Technic bit
#define RTB_OFFSET 4
#define RTB_SSAO (RTB_OFFSET + 0)
#define RTB_SHADOWMAPPING (RTB_OFFSET + 1)
#define RTB_NORMALMAPPING (RTB_OFFSET + 2)


