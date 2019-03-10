#ifndef LEXGINE_CORE_DX_D3D12_CONSTANT_BUFFER_DATA_MAPPER_H
#define LEXGINE_CORE_DX_D3D12_CONSTANT_BUFFER_DATA_MAPPER_H

#include <map>
#include "lexgine/core/misc/hashed_string.h"
#include "lexgine/core/math/vector_types.h"
#include "lexgine/core/math/matrix_types.h"
#include "constant_buffer_reflection.h"

namespace lexgine::core::dx::d3d12 {

template<ConstantBufferReflection::ReflectionEntryBaseType base_type>
struct ReflectionToStaticCast;

template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::float1> { using type = float; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::int1> { using type = int; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::uint1> { using type = unsigned int; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::bool1> { using type = bool; };

template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::float2> { using type = math::Vector2f; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::int2> { using type = math::Vector2i; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::uint2> { using type = math::Vector2u; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::bool2> { using type = math::Vector2b; };

template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::float3> { using type = math::Vector3f; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::int3> { using type = math::Vector3i; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::uint3> { using type = math::Vector3u; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::bool3> { using type = math::Vector3b; };

template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::float4> { using type = math::Vector4f; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::int4> { using type = math::Vector4i; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::uint4> { using type = math::Vector4u; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::bool4> { using type = math::Vector4b; };

template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::float2x2> { using type = math::Matrix2f; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::float2x3> { using type = math::Matrix2x3f; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::float2x4> { using type = math::Matrix2x4f; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::int2x2> { using type = math::Matrix2i; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::int2x3> { using type = math::Matrix2x3i; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::int2x4> { using type = math::Matrix2x4i; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::uint2x2> { using type = math::Matrix2u; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::uint2x3> { using type = math::Matrix2x3u; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::uint2x4> { using type = math::Matrix2x4u; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::bool2x2> { using type = math::Matrix2b; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::bool2x3> { using type = math::Matrix2x3b; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::bool2x4> { using type = math::Matrix2x4b; };

template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::float3x2> { using type = math::Matrix3x2f; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::float3x3> { using type = math::Matrix3f; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::float3x4> { using type = math::Matrix3x4f; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::int3x2> { using type = math::Matrix3x2i; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::int3x3> { using type = math::Matrix3i; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::int3x4> { using type = math::Matrix3x4i; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::uint3x2> { using type = math::Matrix3x2u; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::uint3x3> { using type = math::Matrix3u; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::uint3x4> { using type = math::Matrix3x4u; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::bool3x2> { using type = math::Matrix3x2b; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::bool3x3> { using type = math::Matrix3b; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::bool3x4> { using type = math::Matrix3x4b; };

template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::float4x2> { using type = math::Matrix4x2f; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::float4x3> { using type = math::Matrix4x3f; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::float4x4> { using type = math::Matrix4f; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::int4x2> { using type = math::Matrix4x2i; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::int4x3> { using type = math::Matrix4x3i; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::int4x4> { using type = math::Matrix4i; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::uint4x2> { using type = math::Matrix4x2u; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::uint4x3> { using type = math::Matrix4x3u; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::uint4x4> { using type = math::Matrix4u; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::bool4x2> { using type = math::Matrix4x2b; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::bool4x3> { using type = math::Matrix4x3b; };
template<> struct ReflectionToStaticCast<ConstantBufferReflection::ReflectionEntryBaseType::bool4x4> { using type = math::Matrix4b; };


class ConstantBufferDataMapper final
{
private:
    
};

}

#endif