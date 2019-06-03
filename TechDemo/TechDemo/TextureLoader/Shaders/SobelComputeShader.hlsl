Texture2D gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0);


float CalcLuminance(float3 color)
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
    //return dot(color, float3(1.0f, 1.0f, 1.0f));
}

[numthreads(16, 16, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
   
    float4 c[3][3];

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            int2 xy = DTid.xy + int2(-1 + j, -1 + i);
            c[i][j] = gInput[xy];
        }
    }

    float Gx = -1.0f * c[0][0] - 2.0f * c[1][0] - 1.0f * c[2][0] + 1.0f * c[0][2] + 2.0f * c[1][2] + 1.0f * c[2][2];
    float Gy = -1.0f * c[2][0] - 2.0f * c[2][1] - 1.0f * c[2][2] + 1.0f * c[0][0] + 2.0f * c[0][1] + 1.0f * c[0][2];

    float4 mag = sqrt(Gx * Gx + Gx * Gx);
    mag = 1.0f- saturate(CalcLuminance(mag.rgb));

    gOutput[DTid.xy] =  mag;
}