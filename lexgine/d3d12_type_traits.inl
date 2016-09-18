#ifndef LEXGINE_CORE_MISC_D3D12_TYPE_TRAITS_INL

//! Helper: converts static representation of a format to the corresponding DXGI_FORMAT value.
//! NOTE that not all formats are supported nor are all of them listed in the partial specializations below.
//! Add partial specializations on "when it is needed" basis
template<typename T, unsigned char num_components = 1U, bool half_precision = false, bool normalized = true> struct d3d12_type_traits;

// ********** floating point formats **********
template<bool normalized>
struct d3d12_type_traits<float, 1U, false, normalized>
{
    using iso_c_type = float;
    unsigned char const num_components = 1U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32_FLOAT;
};

template<bool normalized>
struct d3d12_type_traits<float, 1U, true, normalized>
{
    using iso_c_type = float;
    unsigned char const num_components = 1U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16_FLOAT;
};


template<bool normalized>
struct d3d12_type_traits<float, 2U, false, normalized>
{
    using iso_c_type = float;
    unsigned char const num_components = 2U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32G32_FLOAT;
};

template<bool normalized>
struct d3d12_type_traits<float, 2U, true, normalized>
{
    using iso_c_type = float;
    unsigned char const num_components = 2U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16_FLOAT;
};


template<bool normalized>
struct d3d12_type_traits<float, 3U, false, normalized>
{
    using iso_c_type = float;
    unsigned char const num_components = 3U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32G32B32_FLOAT;
};


template<bool normalized>
struct d3d12_type_traits<float, 4U, false, normalized>
{
    using iso_c_type = float;
    unsigned char const num_components = 4U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32G32B32A32_FLOAT;
};

template<bool normalized>
struct d3d12_type_traits<float, 4U, true, normalized>
{
    using iso_c_type = float;
    unsigned char const num_components = 4U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16B16A16_FLOAT;
};
// ********************************************


// ************* integer formats **************
template<bool half_precision>
struct d3d12_type_traits<uint32_t, 1U, half_precision, false>
{
    using iso_c_type = uint32_t;
    unsigned char const num_components = 1U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32_UINT;
};

template<bool half_precision>
struct d3d12_type_traits<int32_t, 1U, half_precision, false>
{
    using iso_c_type = int32_t;
    unsigned char const num_components = 1U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32_SINT;
};

template<bool half_precision>
struct d3d12_type_traits<uint16_t, 1U, half_precision, false>
{
    using iso_c_type = uint16_t;
    unsigned char const num_components = 1U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16_UINT;
};

template<bool half_precision>
struct d3d12_type_traits<uint16_t, 1U, half_precision, true>
{
    using iso_c_type = uint16_t;
    unsigned char const num_components = 1U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16_UNORM;
};

template<bool half_precision>
struct d3d12_type_traits<int16_t, 1U, half_precision, false>
{
    using iso_c_type = int16_t;
    unsigned char const num_components = 1U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16_SINT;
};

template<bool half_precision>
struct d3d12_type_traits<int16_t, 1U, half_precision, true>
{
    using iso_c_type = int16_t;
    unsigned char const num_components = 1U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16_SNORM;
};



template<bool half_precision>
struct d3d12_type_traits<uint32_t, 2U, half_precision, false>
{
    using iso_c_type = uint32_t;
    unsigned char const num_components = 2U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32G32_UINT;
};

template<bool half_precision>
struct d3d12_type_traits<int32_t, 2U, half_precision, false>
{
    using iso_c_type = int32_t;
    unsigned char const num_components = 2U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32G32_SINT;
};

template<bool half_precision>
struct d3d12_type_traits<uint16_t, 2U, half_precision, false>
{
    using iso_c_type = uint16_t;
    unsigned char const num_components = 2U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16_UINT;
};

template<bool half_precision>
struct d3d12_type_traits<uint16_t, 2U, half_precision, true>
{
    using iso_c_type = uint16_t;
    unsigned char const num_components = 2U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16_UNORM;
};

template<bool half_precision>
struct d3d12_type_traits<int16_t, 2U, half_precision, false>
{
    using iso_c_type = int16_t;
    unsigned char const num_components = 2U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16_SINT;
};

template<bool half_precision>
struct d3d12_type_traits<int16_t, 2U, half_precision, true>
{
    using iso_c_type = int16_t;
    unsigned char const num_components = 2U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16_SNORM;
};



template<bool half_precision>
struct d3d12_type_traits<uint32_t, 3U, half_precision, false>
{
    using iso_c_type = uint32_t;
    unsigned char const num_components = 3U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32G32B32_UINT;
};

template<bool half_precision>
struct d3d12_type_traits<int32_t, 3U, half_precision, false>
{
    using iso_c_type = int32_t;
    unsigned char const num_components = 3U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32G32B32_SINT;
};



template<bool half_precision>
struct d3d12_type_traits<uint32_t, 4U, half_precision, false>
{
    using iso_c_type = uint32_t;
    unsigned char const num_components = 4U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32G32B32A32_UINT;
};

template<bool half_precision>
struct d3d12_type_traits<int32_t, 4U, half_precision, false>
{
    using iso_c_type = int32_t;
    unsigned char const num_components = 4U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R32G32B32A32_SINT;
};

template<bool half_precision>
struct d3d12_type_traits<uint16_t, 4U, half_precision, false>
{
    using iso_c_type = uint16_t;
    unsigned char const num_components = 4U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16B16A16_UINT;
};

template<bool half_precision>
struct d3d12_type_traits<uint16_t, 4U, half_precision, true>
{
    using iso_c_type = uint16_t;
    unsigned char const num_components = 4U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16B16A16_UNORM;
};

template<bool half_precision>
struct d3d12_type_traits<int16_t, 4U, half_precision, false>
{
    using iso_c_type = int16_t;
    unsigned char const num_components = 4U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16B16A16_SINT;
};

template<bool half_precision>
struct d3d12_type_traits<int16_t, 4U, half_precision, true>
{
    using iso_c_type = int16_t;
    unsigned char const num_components = 4U;
    DXGI_FORMAT const dxgi_format = DXGI_FORMAT_R16G16B16A16_SNORM;
};
// ********************************************


#define LEXGINE_CORE_MISC_D3D12_TYPE_TRAITS_INL
#endif