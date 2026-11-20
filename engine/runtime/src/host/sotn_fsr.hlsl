// Integração FSR1: EASU e RCAS oficiais AMD, sem substituir os algoritmos.
#define A_GPU 1
#define A_HLSL 1
#include "../../third_party/fsr1/ffx_a.h"

Texture2D gameTexture : register(t0, space2);
SamplerState gameSampler : register(s0, space2);
cbuffer FsrParameters : register(b0, space3) {
    uint4 con0;
    uint4 con1;
    uint4 con2;
    uint4 con3;
    float4 dimensions; // entrada XY, saída ZW.
};

#ifdef SOTN_EASU
#define FSR_EASU_F 1
#else
#define FSR_RCAS_F 1
#define FSR_RCAS_PASSTHROUGH_ALPHA 1
#endif
#include "../../third_party/fsr1/ffx_fsr1.h"

#ifdef SOTN_EASU
AF4 FsrEasuRF(AF2 p) { return gameTexture.GatherRed(gameSampler, p); }
AF4 FsrEasuGF(AF2 p) { return gameTexture.GatherGreen(gameSampler, p); }
AF4 FsrEasuBF(AF2 p) { return gameTexture.GatherBlue(gameSampler, p); }
#else
AF4 FsrRcasLoadF(ASU2 p) {
    int2 bounded = clamp(p, int2(0, 0), int2(dimensions.xy) - 1);
    return gameTexture.Load(int3(bounded, 0));
}
void FsrRcasInputF(inout AF1 r, inout AF1 g, inout AF1 b) {}
#endif

float4 main(float4 tint : TEXCOORD0, float2 uv : TEXCOORD1) : SV_Target {
    uint2 pixel = min(uint2(uv * dimensions.zw), uint2(dimensions.zw) - 1);
    float3 color;
    float alpha;
#ifdef SOTN_EASU
    FsrEasuF(color, pixel, con0, con1, con2, con3);
    // FSR1 opera em RGB. O alfa acompanha a amostragem linear, sem inventar detalhes.
    alpha = gameTexture.SampleLevel(gameSampler, uv, 0).a;
#else
    FsrRcasF(color.r, color.g, color.b, alpha, pixel, con0);
#endif
    return float4(color, alpha) * tint;
}
