#ifndef LEXGINE_CORE_DX_D3D12_CONSTANT_BUFFER_DATA_MAPPER_H
#define LEXGINE_CORE_DX_D3D12_CONSTANT_BUFFER_DATA_MAPPER_H

#include <list>
#include "lexgine/core/misc/hashed_string.h"
#include "lexgine/core/misc/static_vector.h"
#include "lexgine/core/math/vector_types.h"
#include "lexgine/core/math/matrix_types.h"
#include "constant_buffer_reflection.h"

namespace lexgine::core::dx::d3d12 {


template<typename T> struct StaticTypeToReflectionEntryBaseType;

template<> struct StaticTypeToReflectionEntryBaseType<float>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::float1;
};

template<> struct StaticTypeToReflectionEntryBaseType<int>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::int1;
};

template<> struct StaticTypeToReflectionEntryBaseType<unsigned int>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::uint1;
};

template<> struct StaticTypeToReflectionEntryBaseType<bool>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::bool1;
};



template<> struct StaticTypeToReflectionEntryBaseType<math::Vector2f>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::float2;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Vector2i>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::int2;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Vector2u>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::uint2;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Vector2b>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::bool2;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix2f>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::float2x2;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix2i>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::int2x2;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix2u>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::uint2x2;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix2b>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::bool2x2;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix2x3f>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::float2x3;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix2x3i>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::int2x3;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix2x3u>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::uint2x3;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix2x3b>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::bool2x3;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix2x4f>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::float2x4;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix2x4i>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::int2x4;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix2x4u>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::uint2x4;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix2x4b>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::bool2x4;
};



template<> struct StaticTypeToReflectionEntryBaseType<math::Vector3f>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::float3;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Vector3i>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::int3;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Vector3u>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::uint3;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Vector3b>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::bool3;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix3f>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::float3x3;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix3i>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::int3x3;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix3u>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::uint3x3;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix3b>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::bool3x3;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix3x2f>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::float3x2;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix3x2i>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::int3x2;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix3x2u>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::uint3x2;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix3x2b>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::bool3x2;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix3x4f>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::float3x4;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix3x4i>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::int3x4;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix3x4u>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::uint3x4;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix3x4b>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::bool3x4;
};



template<> struct StaticTypeToReflectionEntryBaseType<math::Vector4f>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::float4;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Vector4i>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::int4;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Vector4u>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::uint4;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Vector4b>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::bool4;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix4f>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::float4x4;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix4i>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::int4x4;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix4u>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::uint4x4;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix4b>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::bool4x4;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix4x2f>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::float4x2;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix4x2i>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::int4x2;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix4x2u>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::uint4x2;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix4x2b>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::bool4x2;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix4x3f>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::float4x3;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix4x3i>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::int4x3;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix4x3u>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::uint4x3;
};

template<> struct StaticTypeToReflectionEntryBaseType<math::Matrix4x3b>
{
    static ConstantBufferReflection::ReflectionEntryBaseType constexpr base_reflection_type = ConstantBufferReflection::ReflectionEntryBaseType::bool4x3;
};


class AbstractConstantDataProvider
{
public:
    virtual void const* data() const = 0;
    virtual size_t elementCount() const = 0;
    virtual ConstantBufferReflection::ReflectionEntryBaseType reflectionType() const = 0;
};

template<typename T> class ConstantDataProvider : public AbstractConstantDataProvider
{
public:
    using value_type = typename std::conditional<std::is_scalar<T>::value,
        T,
        typename std::remove_const<typename std::remove_reference<T>::type>::type&>;

public:
    explicit ConstantDataProvider(typename std::remove_reference<value_type>::type const& value)
        : m_value{ value }
    {

    }

    void const* data() const override { return &m_value; }
    size_t elementCount() const override { return 1U; }

    ConstantBufferReflection::ReflectionEntryBaseType reflectionType() const override
    {
        return StaticTypeToReflectionEntryBaseType<value_type>::base_reflection_type;
    }

    typename value_type& value() { return m_value; }
    typename std::remove_reference<value_type>::type const& value() const { return m_value; }

private:
    value_type m_value;
};

template<typename T> class ConstantDataProvider<std::vector<T>> : public AbstractConstantDataProvider
{
public:
    explicit ConstantDataProvider(std::vector<T> const& value)
        : m_value{ value }
    {

    }

    void const* data() const override { return m_value.data(); }
    size_t elementCount() const override { return m_value.size(); }

    std::vector<T>& value() { return m_value; }
    std::vector<T> const& value() const { return m_value; }
    ConstantBufferReflection::ReflectionEntryBaseType reflectionType() const override
    {
        return
            StaticTypeToReflectionEntryBaseType<typename std::remove_const<typename std::remove_reference<T>::type>::type>
            ::base_reflection_type;
    }

private:
    std::vector<T>& m_value;
};



class ConstantBufferDataUpdater
{
public:
    ConstantBufferDataUpdater(std::string const& variable_name,
        std::shared_ptr<AbstractConstantDataProvider> const& constant_data_provider);

    void update(uint64_t constant_buffer_allocation_base_address, ConstantBufferReflection const& reflection) const;

protected:
    misc::HashedString m_hash;
    std::weak_ptr<AbstractConstantDataProvider> m_constant_data_provider_ptr;
};


class ConstantBufferDataMapper final
{
public:
    ConstantBufferDataMapper(ConstantBufferReflection const& reflection);

    void addDataUpdater(ConstantBufferDataUpdater const& data_updater);

    size_t requiredDestinationBufferCapacity() const;

    void update(uint64_t constant_buffer_allocation_base_address) const;

private:
    ConstantBufferReflection const& m_reflection;
    std::list<ConstantBufferDataUpdater> m_updaters;
};

}

#endif