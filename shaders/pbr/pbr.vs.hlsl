#include "common/environment.hlsli"

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
    float4 tangent : TANGENT;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float3 tangent : TEXCOORD3;
    float3 bitangent : TEXCOORD4;
};

VSOutput VSMain(VSInput input) {
    VSOutput output;

    float4 localPos = float4(input.position, 1.0f);
    float4 worldPos = mul(cameraData.model, localPos);
    output.position = mul(cameraData.projection, mul(cameraData.view, worldPos));
    output.worldPos = worldPos.xyz;

    float3 N = mul((float3x3)cameraData.model, input.normal);
    float3 T = normalize(mul((float3x3)cameraData.model, input.tangent.xyz));
    float3 B = normalize(cross(N, T) * input.tangent.w);

    output.normal = normalize(N);
    output.tangent = T;
    output.bitangent = B;
    output.uv = input.texcoord;

    return output;
}
