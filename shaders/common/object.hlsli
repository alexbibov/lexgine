#ifndef OBJECT_HLSLI
#define OBJECT_HLSLI

#include "common/common.hlsli"

struct ObjectData {
    float4x4 model;
};

ConstantBuffer<ObjectData> object_data : register(OBJECT_DATA_REGISTER, SHADER_FUNCTION_SPACE);

#endif