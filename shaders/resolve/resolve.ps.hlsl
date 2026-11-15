#include "common/common.hlsli"

Texture2D gbuffer_albedo : register(t0, PASS_SPACE);
SamplerState linear_sampler : register(s0, PASS_SPACE);

struct VSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 PSMain(VSInput input) : SV_Target0 {
    return float4(gbuffer_albedo.Sample(linear_sampler, input.uv).rgb, 1.f);
}
