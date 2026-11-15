#ifndef OBJECT_HLSLI
#define OBJECT_HLSLI

#include "common/common.hlsli"

struct ObjectData {
    float4x4 model;
};

StructuredBuffer<ObjectData> object_data : register(INSTANCE_DATA_REGISTER, SHADER_FUNCTION_SPACE);

#endif