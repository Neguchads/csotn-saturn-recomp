Texture2D gameTexture : register(t0, space2);
SamplerState gameSampler : register(s0, space2);

cbuffer EffectParameters : register(b0, space3) {
    float frameWidth;
    float frameHeight;
    float effectMode;
    float padding;
};

float4 neighbor(int2 coordinate, int2 size) {
    int2 bounded = clamp(coordinate, int2(0, 0), size - 1);
    return gameTexture.SampleLevel(gameSampler, (float2(bounded) + 0.5) / float2(size), 0.0);
}

bool same(float4 a, float4 b) { return all(a == b); }

// Implementacao propria das regras publicadas em https://www.scale2x.it/algorithm.
float4 upscale(float2 uv, int scale) {
    int2 size = int2(frameWidth, frameHeight);
    float2 sourcePosition = uv * float2(size);
    int2 p = int2(floor(sourcePosition));
    int2 cell = min(int2(frac(sourcePosition) * scale), scale - 1);
    float4 b = neighbor(p + int2(0, -1), size);
    float4 d = neighbor(p + int2(-1, 0), size);
    float4 e = neighbor(p, size);
    float4 f = neighbor(p + int2(1, 0), size);
    float4 h = neighbor(p + int2(0, 1), size);
    if (same(b, h) || same(d, f)) return e;
    if (scale == 2) {
        if (cell.y == 0) return cell.x == 0 ? (same(d, b) ? d : e) : (same(b, f) ? f : e);
        return cell.x == 0 ? (same(d, h) ? d : e) : (same(h, f) ? f : e);
    }
    float4 a = neighbor(p + int2(-1, -1), size);
    float4 c = neighbor(p + int2(1, -1), size);
    float4 g = neighbor(p + int2(-1, 1), size);
    float4 i = neighbor(p + int2(1, 1), size);
    if (cell.y == 0) {
        if (cell.x == 0) return same(d, b) ? d : e;
        if (cell.x == 2) return same(b, f) ? f : e;
        return ((same(d, b) && !same(e, c)) || (same(b, f) && !same(e, a))) ? b : e;
    }
    if (cell.y == 2) {
        if (cell.x == 0) return same(d, h) ? d : e;
        if (cell.x == 2) return same(h, f) ? f : e;
        return ((same(d, h) && !same(e, i)) || (same(h, f) && !same(e, g))) ? h : e;
    }
    if (cell.x == 0) return ((same(d, b) && !same(e, g)) || (same(d, h) && !same(e, a))) ? d : e;
    if (cell.x == 2) return ((same(b, f) && !same(e, i)) || (same(h, f) && !same(e, c))) ? f : e;
    return e;
}

// O DXIL distribuído pelo SDL usa TEXCOORD0 para cor e TEXCOORD1 para UV.
float4 main(float4 tint : TEXCOORD0, float2 uv : TEXCOORD1) : SV_Target {
    if (effectMode > 2.5) return upscale(uv, effectMode < 3.5 ? 2 : 3) * tint;
    float4 color = gameTexture.Sample(gameSampler, uv) * tint;
    if (effectMode < 1.5) {
        float row = fmod(floor(uv.y * frameHeight), 2.0);
        color.rgb *= lerp(1.0, 0.75, row);
        float2 centered = uv * 2.0 - 1.0;
        color.rgb *= saturate(1.0 - 0.12 * dot(centered, centered));
    } else {
        float column = fmod(floor(uv.x * frameWidth), 3.0);
        float3 mask = column < 0.5 ? float3(1.0, 0.86, 0.86)
                    : column < 1.5 ? float3(0.86, 1.0, 0.86)
                                   : float3(0.86, 0.86, 1.0);
        color.rgb *= mask;
    }
    return color;
}
