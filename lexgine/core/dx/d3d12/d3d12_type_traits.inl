#ifndef LEXGINE_CORE_DX_D3D12_TYPE_TRAITS_INL

namespace lexgine { namespace core { namespace dx { namespace d3d12 {

//! Helper: converts static representation of a format to the corresponding DXGI_FORMAT value.
//! NOTE that not all formats are supported nor are all of them listed in the partial specializations below.
//! Add partial specializations on "when it is needed" basis
template<typename T, unsigned char num_components = 1U, bool half_precision = false, bool normalized = true> 
struct d3d12_type_traits
{
    using iso_c_type = void;
    static unsigned char const num_components = 0;
    static unsigned char const total_size_in_bytes = 4U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_UNKNOWN;
};

// ********** floating point formats **********
template<bool normalized>
struct d3d12_type_traits<float, 1U, false, normalized>
{
    using iso_c_type = float;
    static unsigned char const num_components = 1U;
    static unsigned char const total_size_in_bytes = 4U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32_FLOAT;
};

template<bool normalized>
struct d3d12_type_traits<float, 1U, true, normalized>
{
    using iso_c_type = float;
    static unsigned char const num_components = 1U;
    static unsigned char const total_size_in_bytes = 2U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16_FLOAT;
};


template<bool normalized>
struct d3d12_type_traits<float, 2U, false, normalized>
{
    using iso_c_type = float;
    static unsigned char const num_components = 2U;
    static unsigned char const total_size_in_bytes = 8U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32G32_FLOAT;
};

template<bool normalized>
struct d3d12_type_traits<float, 2U, true, normalized>
{
    using iso_c_type = float;
    static unsigned char const num_components = 2U;
    static unsigned char const total_size_in_bytes = 4U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16_FLOAT;
};


template<bool normalized>
struct d3d12_type_traits<float, 3U, false, normalized>
{
    using iso_c_type = float;
    static unsigned char const num_components = 3U;
    static unsigned char const total_size_in_bytes = 12U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32G32B32_FLOAT;
};


template<bool normalized>
struct d3d12_type_traits<float, 4U, false, normalized>
{
    using iso_c_type = float;
    static unsigned char const num_components = 4U;
    static unsigned char const total_size_in_bytes = 16U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32G32B32A32_FLOAT;
};

template<bool normalized>
struct d3d12_type_traits<float, 4U, true, normalized>
{
    using iso_c_type = float;
    static unsigned char const num_components = 4U;
    static unsigned char const total_size_in_bytes = 8U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16B16A16_FLOAT;
};
// ********************************************


// ************* integer formats **************
template<bool half_precision>
struct d3d12_type_traits<uint32_t, 1U, half_precision, false>
{
    using iso_c_type = uint32_t;
    static unsigned char const num_components = 1U;
    static unsigned char const total_size_in_bytes = 4U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32_UINT;
};

template<bool half_precision>
struct d3d12_type_traits<int32_t, 1U, half_precision, false>
{
    using iso_c_type = int32_t;
    static unsigned char const num_components = 1U;
    static unsigned char const total_size_in_bytes = 4U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32_SINT;
};

template<bool half_precision>
struct d3d12_type_traits<uint16_t, 1U, half_precision, false>
{
    using iso_c_type = uint16_t;
    static unsigned char const num_components = 1U;
    static unsigned char const total_size_in_bytes = 2U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16_UINT;
};

template<bool half_precision>
struct d3d12_type_traits<uint16_t, 1U, half_precision, true>
{
    using iso_c_type = uint16_t;
    static unsigned char const num_components = 1U;
    static unsigned char const total_size_in_bytes = 2U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16_UNORM;
};

template<bool half_precision>
struct d3d12_type_traits<int16_t, 1U, half_precision, false>
{
    using iso_c_type = int16_t;
    static unsigned char const num_components = 1U;
    static unsigned char const total_size_in_bytes = 2U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16_SINT;
};

template<bool half_precision>
struct d3d12_type_traits<int16_t, 1U, half_precision, true>
{
    using iso_c_type = int16_t;
    static unsigned char const num_components = 1U;
    static unsigned char const total_size_in_bytes = 2U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16_SNORM;
};



template<bool half_precision>
struct d3d12_type_traits<uint32_t, 2U, half_precision, false>
{
    using iso_c_type = uint32_t;
    static unsigned char const num_components = 2U;
    static unsigned char const total_size_in_bytes = 8U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32G32_UINT;
};

template<bool half_precision>
struct d3d12_type_traits<int32_t, 2U, half_precision, false>
{
    using iso_c_type = int32_t;
    static unsigned char const num_components = 2U;
    static unsigned char const total_size_in_bytes = 8U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32G32_SINT;
};

template<bool half_precision>
struct d3d12_type_traits<uint16_t, 2U, half_precision, false>
{
    using iso_c_type = uint16_t;
    static unsigned char const num_components = 2U;
    static unsigned char const total_size_in_bytes = 4U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16_UINT;
};

template<bool half_precision>
struct d3d12_type_traits<uint16_t, 2U, half_precision, true>
{
    using iso_c_type = uint16_t;
    static unsigned char const num_components = 2U;
    static unsigned char const total_size_in_bytes = 4U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16_UNORM;
};

template<bool half_precision>
struct d3d12_type_traits<int16_t, 2U, half_precision, false>
{
    using iso_c_type = int16_t;
    static unsigned char const num_components = 2U;
    static unsigned char const total_size_in_bytes = 4U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16_SINT;
};

template<bool half_precision>
struct d3d12_type_traits<int16_t, 2U, half_precision, true>
{
    using iso_c_type = int16_t;
    static unsigned char const num_components = 2U;
    static unsigned char const total_size_in_bytes = 4U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16_SNORM;
};



template<bool half_precision>
struct d3d12_type_traits<uint32_t, 3U, half_precision, false>
{
    using iso_c_type = uint32_t;
    static unsigned char const num_components = 3U;
    static unsigned char const total_size_in_bytes = 12U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32G32B32_UINT;
};

template<bool half_precision>
struct d3d12_type_traits<int32_t, 3U, half_precision, false>
{
    using iso_c_type = int32_t;
    static unsigned char const num_components = 3U;
    static unsigned char const total_size_in_bytes = 12U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32G32B32_SINT;
};



template<bool half_precision>
struct d3d12_type_traits<uint32_t, 4U, half_precision, false>
{
    using iso_c_type = uint32_t;
    static unsigned char const num_components = 4U;
    static unsigned char const total_size_in_bytes = 16U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32G32B32A32_UINT;
};

template<bool half_precision>
struct d3d12_type_traits<int32_t, 4U, half_precision, false>
{
    using iso_c_type = int32_t;
    static unsigned char const num_components = 4U;
    static unsigned char const total_size_in_bytes = 16U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32G32B32A32_SINT;
};

template<bool half_precision>
struct d3d12_type_traits<uint16_t, 4U, half_precision, false>
{
    using iso_c_type = uint16_t;
    static unsigned char const num_components = 4U;
    static unsigned char const total_size_in_bytes = 8U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16B16A16_UINT;
};

template<bool half_precision>
struct d3d12_type_traits<uint16_t, 4U, half_precision, true>
{
    using iso_c_type = uint16_t;
    static unsigned char const num_components = 4U;
    static unsigned char const total_size_in_bytes = 8U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16B16A16_UNORM;
};

template<bool half_precision>
struct d3d12_type_traits<int16_t, 4U, half_precision, false>
{
    using iso_c_type = int16_t;
    static unsigned char const num_components = 4U;
    static unsigned char const total_size_in_bytes = 8U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16B16A16_SINT;
};

template<bool half_precision>
struct d3d12_type_traits<int16_t, 4U, half_precision, true>
{
    using iso_c_type = int16_t;
    static unsigned char const num_components = 4U;
    static unsigned char const total_size_in_bytes = 8U;
    static DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16B16A16_SNORM;
};
// ********************************************


template<misc::DataFormat data_format>
struct DataFormatToStaticType;

template<>
struct DataFormatToStaticType<misc::DataFormat::float64>
{
    using value_type = double;
};

template<>
struct DataFormatToStaticType<misc::DataFormat::float32>
{
    using value_type = float;
};


// NOTE: implement half-precision floating-point arithmetics
template<>
struct DataFormatToStaticType<misc::DataFormat::float16>
{
    using value_type = half;
};

template<>
struct DataFormatToStaticType<misc::DataFormat::int64>
{
    using value_type = std::int64_t;
};

template<>
struct DataFormatToStaticType<misc::DataFormat::int32>
{
    using value_type = std::int32_t;
};

template<>
struct DataFormatToStaticType<misc::DataFormat::int16>
{
    using value_type = std::int16_t;
};

template<>
struct DataFormatToStaticType<misc::DataFormat::uint64>
{
    using value_type = std::uint64_t;
};

template<>
struct DataFormatToStaticType<misc::DataFormat::uint32>
{
    using value_type = std::uint32_t;
};

template<>
struct DataFormatToStaticType<misc::DataFormat::uint16>
{
    using value_type = std::uint16_t;
};

template<>
struct DataFormatToStaticType<misc::DataFormat::unknown>
{
    using value_type = void*;
};


template<typename T>
struct StaticTypeToDataFormat
{
    static misc::DataFormat const data_format = misc::DataFormat::unknown;
};

template<>
struct StaticTypeToDataFormat<double>
{
    static misc::DataFormat const data_format = misc::DataFormat::float64;
};

template<>
struct StaticTypeToDataFormat<float>
{
    static misc::DataFormat const data_format = misc::DataFormat::float32;
};

template<>
struct StaticTypeToDataFormat<half>
{
    static misc::DataFormat const data_format = misc::DataFormat::float16;
};


template<>
struct StaticTypeToDataFormat<int64_t>
{
    static misc::DataFormat const data_format = misc::DataFormat::int64;
};

template<>
struct StaticTypeToDataFormat<int32_t>
{
    static misc::DataFormat const data_format = misc::DataFormat::int32;
};

template<>
struct StaticTypeToDataFormat<int16_t>
{
    static misc::DataFormat const data_format = misc::DataFormat::int16;
};


template<>
struct StaticTypeToDataFormat<uint64_t>
{
    static misc::DataFormat const data_format = misc::DataFormat::uint64;
};

template<>
struct StaticTypeToDataFormat<uint32_t>
{
    static misc::DataFormat const data_format = misc::DataFormat::uint32;
};

template<>
struct StaticTypeToDataFormat<uint16_t>
{
    static misc::DataFormat const data_format = misc::DataFormat::uint16;
};

}}}}

#define LEXGINE_CORE_DX_D3D12_TYPE_TRAITS_INL
#endif