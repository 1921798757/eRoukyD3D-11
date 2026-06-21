#include "Cube.hlsli"

float4 PS(VertexOut pIn) : SV_Target
{
    return g_FaceTexture.Sample(g_SamLinear, pIn.tex);
}