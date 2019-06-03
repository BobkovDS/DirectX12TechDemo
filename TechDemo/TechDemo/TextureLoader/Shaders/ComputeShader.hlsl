struct inData
{
    float3 coords; 
    float w;
};

struct outData
{
    float l;    
};

//StructuredBuffer<inData> gInput : register(t0);
//RWStructuredBuffer<outData> gOutput : register(u0);
//-------------
//ConsumeStructuredBuffer<inData> gInput : register(u0);
//AppendStructuredBuffer<outData> gOutput : register(u1);
//RWBuffer<int> gCounter : register(u2);

//-------------
// Blur Computer Shader

static const int gBlurRadius =4;
#define ThreadCountX 256
#define ThreadCountY 256
#define CacheSize (ThreadCountX + 2*gBlurRadius)

Texture2D inTexture : register(t0);
RWTexture2D<float4> outTexture : register(u0);
groupshared float4 gGroupCache[CacheSize];

[numthreads(ThreadCountX, 1, 1)]
void CS(int3 dispatchThreadID : SV_DispatchThreadID,
                int3 groupThreadID: SV_GroupThreadID)
{        
    int inWidth = 0;
    int inHeight = 0;
    inTexture.GetDimensions(inWidth, inHeight);

    float weight[9] = { 0.04f, 0.06f, 0.08f, 0.12f, 0.4f, 0.12f, 0.08f, 0.06f, 0.04 };

    //add additional values to cache in left side
    if (groupThreadID.x < gBlurRadius)
    {
        int x = max(dispatchThreadID.x - gBlurRadius, 0);
        gGroupCache[groupThreadID.x] = inTexture[int2(x, dispatchThreadID.y)];
    }

    //add additional values to cache in right side
    if (groupThreadID.x >= (ThreadCountX - gBlurRadius))
    {
        int x = min(dispatchThreadID.x + gBlurRadius, inWidth - 1);
        gGroupCache[groupThreadID.x + 2 * gBlurRadius] = inTexture[int2(x, dispatchThreadID.y)];
    }       
    
    gGroupCache[groupThreadID.x + gBlurRadius] = inTexture[min(dispatchThreadID.xy, int2(inWidth-1, inHeight-1))];
        
    GroupMemoryBarrierWithGroupSync();
        
    float4 color = { 0.0f, 0.0f, 0.0f, 0.0f };

    if ((dispatchThreadID.x < 100) || (dispatchThreadID.x > 500))
    {
        for (int i = -gBlurRadius; i <= gBlurRadius; i++)
        {
            int k = groupThreadID.x + gBlurRadius + i;

            color = color + weight[i + gBlurRadius] * gGroupCache[k] ;
            color *= float4(0.95f, 1.0f, 0.95f, 1.0f);

        }
    }
    else
    {
        color = gGroupCache[groupThreadID.x + gBlurRadius] ;
    }      

    outTexture[dispatchThreadID.xy] = color;// * float4(0.5f, 1.0f, 0.5f, 1.0f);
}

[numthreads(1, ThreadCountY, 1)]
void CS_hor(int3 dispatchThreadID : SV_DispatchThreadID,
                   int3 groupThreadID : SV_GroupThreadID)
{
    int inWidth = 0;
    int inHeight = 0;
    inTexture.GetDimensions(inWidth, inHeight);

    float weight[9] = { 0.04f, 0.06f, 0.08f, 0.12f, 0.4f, 0.12f, 0.08f, 0.06f, 0.04 };

    //add additional values to cache in left side
    if (groupThreadID.y < gBlurRadius)
    {
        int y = max(dispatchThreadID.y - gBlurRadius, 0);
        gGroupCache[groupThreadID.y] = inTexture[int2(dispatchThreadID.x, y)];
    }

    //add additional values to cache in right side
    if (groupThreadID.y >= (ThreadCountY - gBlurRadius))
    {
        int y = min(dispatchThreadID.y + gBlurRadius, inHeight - 1);
        gGroupCache[groupThreadID.y + 2 * gBlurRadius] = inTexture[int2(dispatchThreadID.x, y)];
    }
    
    gGroupCache[groupThreadID.y + gBlurRadius] = inTexture[min(dispatchThreadID.xy, int2(inWidth - 1, inHeight - 1))];
        
    GroupMemoryBarrierWithGroupSync();
        
    float4 color = { 0.0f, 0.0f, 0.0f, 0.0f };

    if ((dispatchThreadID.y < 100) || (dispatchThreadID.y > 500))
    {
        for (int i = -gBlurRadius; i <= gBlurRadius; i++)
        {
            int k = groupThreadID.y + gBlurRadius + i;

            color = color + weight[i + gBlurRadius] * gGroupCache[k];
            color *= float4(0.95f, 1.0f, 0.95f, 1.0f);
        }
    }
    else
        color = gGroupCache[groupThreadID.y + gBlurRadius];

    

    outTexture[dispatchThreadID.xy] = color;
}

/*
[numthreads(ThreadCountX, 1, 1)]
void CS(int3 dispatchThreadID : SV_DispatchThreadID,
                int3 groupThreadID: SV_GroupThreadID)
{        
    int inWidth = 0;
    int inHeight = 0;
    inTexture.GetDimensions(inWidth, inHeight);

    float weight[9] = { 0.04f, 0.06f, 0.08f, 0.12f, 0.4f, 0.12f, 0.08f, 0.06f, 0.04 };

    //add additional values to cache in left side
    if (groupThreadID.x < gBlurRadius)
    {
        int x = max(dispatchThreadID.x - gBlurRadius, 0);
        gGroupCache[groupThreadID.x] = inTexture[int2(x, dispatchThreadID.y)];
    }

    //add additional values to cache in right side
    if (groupThreadID.x >= (ThreadCountX - gBlurRadius))
    {
        int x = min(dispatchThreadID.x + gBlurRadius, inWidth - 1);
        gGroupCache[groupThreadID.x + 2 * gBlurRadius] = inTexture[int2(x, dispatchThreadID.y)];
    }       
    
    gGroupCache[groupThreadID.x + gBlurRadius] = inTexture[min(dispatchThreadID.xy, int2(inWidth-1, inHeight-1))];
        
    GroupMemoryBarrierWithGroupSync();
        
    float4 color = { 0.0f, 0.0f, 0.0f, 0.0f };

    for (int i = -gBlurRadius; i <= gBlurRadius; i++)
    {
        int k = groupThreadID.x + gBlurRadius + i;

        color = color + weight[i+gBlurRadius]*gGroupCache[k];
    }

    outTexture[dispatchThreadID.xy] = color;// * float4(0.5f, 1.0f, 0.5f, 1.0f);
}

[numthreads(1, ThreadCountY, 1)]
void CS_hor(int3 dispatchThreadID : SV_DispatchThreadID,
                   int3 groupThreadID : SV_GroupThreadID)
{
    int inWidth = 0;
    int inHeight = 0;
    inTexture.GetDimensions(inWidth, inHeight);

    float weight[9] = { 0.04f, 0.06f, 0.08f, 0.12f, 0.4f, 0.12f, 0.08f, 0.06f, 0.04 };

    //add additional values to cache in left side
    if (groupThreadID.y < gBlurRadius)
    {
        int y = max(dispatchThreadID.y - gBlurRadius, 0);
        gGroupCache[groupThreadID.y] = inTexture[int2(dispatchThreadID.x, y)];
    }

    //add additional values to cache in right side
    if (groupThreadID.y >= (ThreadCountY - gBlurRadius))
    {
        int y = min(dispatchThreadID.y + gBlurRadius, inHeight - 1);
        gGroupCache[groupThreadID.y + 2 * gBlurRadius] = inTexture[int2(dispatchThreadID.x,y)];
    }       
    
    gGroupCache[groupThreadID.y + gBlurRadius] = inTexture[min(dispatchThreadID.xy, int2(inWidth - 1, inHeight - 1))];
        
    GroupMemoryBarrierWithGroupSync();
        
    float4 color = { 0.0f, 0.0f, 0.0f, 0.0f };

    for (int i = -gBlurRadius; i <= gBlurRadius; i++)
    {
        int k = groupThreadID.y + gBlurRadius + i;

        color = color + weight[i + gBlurRadius] * gGroupCache[k];
    }

    outTexture[dispatchThreadID.xy] = color;// * float4(1.0f, 0.5f, 0.5f, 1.0f);
}
*/