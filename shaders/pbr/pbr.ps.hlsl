#include "common/environment.hlsli"

ConstantBuffer<MaterialTextureIds> materialData : register(b1, space0);

float3 GetNormalFromMap(VSOutput input) {
    float3 tangentNormal = normalTex.Sample(samplerLinear, input.uv).xyz * 2.0 - 1.0;
    float3x3 TBN = float3x3(
        normalize(input.tangent),
        normalize(input.bitangent),
        normalize(input.normal)
    );
    return normalize(mul(tangentNormal, TBN));
}

GBufferOutput PSMain(VSOutput input) {
    GBufferOutput gOut;

    // Sample bindless textures using material-supplied indices
    float3 albedo = materialData.baseColorFactor;
    if(materialData.usageFlags & MATERIAL_DATA_TEXTURE_USAGE_ALBEDO) {
        albedo *= gMaterialTextures[materialData.albedoTexIndex].Sample(gSampler, input.uv).rgb;
    }
   

    // // Normal map (tangent space)
    // float3 tangentNormal = gNormalTextures[materialData.normalTexIndex].Sample(gSampler, input.uv).xyz * 2.0f - 1.0f;
    // float3 normalVS = NormalizeTangentToView(tangentNormal, input.tangent, input.bitangent, input.normal); // Or TBN matrix

    // // Encode normal into octahedral
    // float2 normalOct = EncodeNormalOctahedral(normalVS);

    // // Metallic and roughness (assumed to be in R and G channels)
    // float2 mr = gMetallicRoughness[materialData.mrTexIndex].Sample(gSampler, input.uv).rg;
    // uint metallic = (uint)(mr.r * 255.0f);
    // uint roughness = (uint)(mr.g * 255.0f);
    // uint packedMR = (roughness << 8) | metallic;
    // float packedMRFloat = asfloat(packedMR);

    // // Emissive RGB split
    // float3 emissiveRGB = gEmissiveTextures[emissiveTexIndex].Sample(gSampler, input.uv).rgb;

    // // Populate G-buffer
    // output.albedo = albedo;

    // output.normal = float4(
    //     normalOct,         // x, y
    //     packedMRFloat,     // z (MR packed)
    //     emissiveRGB.r      // a (emissive red)
    // );

    // output.emissive = emissiveRGB.gb; // emissive green and blue

    return output;
}
