#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "common/common.hlsli"

struct EnvironmentData {
    float4x4 view;
    float4x4 projection;
    float4x4 view_projection;
    float4x4 inv_projection;
    float4x4 inv_view_projection
    float3 camera_position;
    float padding; // Align to 16-byte boundary
};

#define MATERIAL_DATA_TEXTURE_USAGE_ALBEDO             0x1
#define MATERIAL_DATA_TEXTURE_USAGE_NORMAL             0x2
#define MATERIAL_DATA_TEXTURE_USAGE_METALLIC_ROUGHNESS 0x4
#define MATERIAL_DATA_TEXTURE_USAGE_EMISSIVE           0x8

ConstantBuffer<EnvironmentData> environment_data : register(b0, SHADER_FUNCTION_SPACE);
SamplerState linear_sampler : register(s0); // Shared sampler

// G-buffer structure (output per-fragment)
struct GBufferOutput {
    float3 albedo : SV_Target0;   // RGB: albedo (diffuse) color; format R11G11B10
    float4 normal : SV_Target1;   // RG: // Normals (RG) in view space encoded using octohedral coordinates, metallic (8 LS-bits) and roughness (8 MS-bits) of B, A is red component of emission; format R16G16B16A16
    float2 emissive : SV_Target2; // RGB: emission color BA-components; format R166B16
};

#endif
