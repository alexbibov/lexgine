#include "common/common.hlsli"

struct VSOutput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

VSOutput VSMain(uint vertex_id : SV_VertexID) {
    // fullscreen triangle: vertex ids 0, 1, 2 map to uv (0, 0), (2, 0), (0, 2)
    float2 uv = float2((vertex_id << 1) & 2, vertex_id & 2);

    VSOutput output;
    output.position = float4(uv * 2.f - 1.f, 0.f, 1.f);
    output.uv = float2(uv.x, 1.f - uv.y);
    return output;
}
