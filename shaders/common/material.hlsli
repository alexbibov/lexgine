#ifndef MATERIAL_H
#define MATERIAL_H

#include "common/common.hlsli"

struct ObjectData {
    float4x4 model;
};

struct MaterialData {
    float3 base_color_factor;
    float metallic_factor;
    
    float3 emissive_factor; 
    float roughness_factor;
    
    uint albedo_tex_index;
    uint normal_tex_index;
    uint mr_tex_index;
    uint emissive_tex_index;
    
    uint alpha_mode;
    uint is_double_sided;
    float alpha_cutoff;
    uint usage_flags; // Bitmask for texture usage
};

ConstantBuffer<ObjectData>   object_data   : register(b1, SHADER_FUNCTION_SPACE)
ConstantBuffer<MaterialData> material_data : register(b2, SHADER_FUNCTION_SPACE);

#endif