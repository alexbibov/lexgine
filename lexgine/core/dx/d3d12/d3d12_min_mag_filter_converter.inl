#ifndef LEXGINE_CORE_DX_D3D12_MIN_MAG_FILTER_CONVERTER_INL

namespace lexgine { namespace core { namespace misc {

template<MinificationFilter min_filter, MagnificationFilter mag_filter, bool is_comparison>
struct MinMagFilterConverter<EngineAPI::Direct3D12, min_filter, mag_filter, is_comparison>
{
    static uint32_t constexpr value() { static_assert(false, "requested combination of minification and magnification filters is not supported by Direct3D 12 API"); }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::nearest, MagnificationFilter::nearest, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MIN_MAG_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::linear, MagnificationFilter::nearest, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::nearest_mipmap_nearest, MagnificationFilter::nearest, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MIN_MAG_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::linear_mipmap_nearest, MagnificationFilter::nearest, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::nearest_mipmap_linear, MagnificationFilter::nearest, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::linear_mipmap_linear, MagnificationFilter::nearest, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR; }
};



template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::nearest, MagnificationFilter::linear, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::linear, MagnificationFilter::linear, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::nearest_mipmap_nearest, MagnificationFilter::linear, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::linear_mipmap_nearest, MagnificationFilter::linear, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::nearest_mipmap_linear, MagnificationFilter::linear, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::linear_mipmap_linear, MagnificationFilter::linear, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MIN_MAG_MIP_LINEAR; }
};



template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::anisotropic, MagnificationFilter::anisotropic, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_ANISOTROPIC; }
};





template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::minimum_nearest, MagnificationFilter::minimum_nearest, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::minimum_linear, MagnificationFilter::minimum_nearest, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::minimum_nearest_mipmap_nearest, MagnificationFilter::minimum_nearest, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::minimum_linear_mipmap_nearest, MagnificationFilter::minimum_nearest, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::minimum_nearest_mipmap_linear, MagnificationFilter::minimum_nearest, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::minimum_linear_mipmap_linear, MagnificationFilter::minimum_nearest, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR; }
};



template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::minimum_nearest, MagnificationFilter::minimum_linear, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::minimum_linear, MagnificationFilter::minimum_linear, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::minimum_nearest_mipmap_nearest, MagnificationFilter::minimum_linear, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::minimum_linear_mipmap_nearest, MagnificationFilter::minimum_linear, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::minimum_nearest_mipmap_linear, MagnificationFilter::minimum_linear, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::minimum_linear_mipmap_linear, MagnificationFilter::minimum_linear, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR; }
};



template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::minimum_anisotropic, MagnificationFilter::minimum_anisotropic, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MINIMUM_ANISOTROPIC; }
};





template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::maximum_nearest, MagnificationFilter::maximum_nearest, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::maximum_linear, MagnificationFilter::maximum_nearest, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::maximum_nearest_mipmap_nearest, MagnificationFilter::maximum_nearest, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::maximum_linear_mipmap_nearest, MagnificationFilter::maximum_nearest, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::maximum_nearest_mipmap_linear, MagnificationFilter::maximum_nearest, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::maximum_linear_mipmap_linear, MagnificationFilter::maximum_nearest, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR; }
};



template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::maximum_nearest, MagnificationFilter::maximum_linear, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::maximum_linear, MagnificationFilter::maximum_linear, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::maximum_nearest_mipmap_nearest, MagnificationFilter::maximum_linear, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::maximum_linear_mipmap_nearest, MagnificationFilter::maximum_linear, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::maximum_nearest_mipmap_linear, MagnificationFilter::maximum_linear, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::maximum_linear_mipmap_linear, MagnificationFilter::maximum_linear, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR; }
};



template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::maximum_anisotropic, MagnificationFilter::maximum_anisotropic, false>
{
    static uint32_t constexpr value() { return D3D12_FILTER_MAXIMUM_ANISOTROPIC; }
};






template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::nearest, MagnificationFilter::nearest, true>
{
    static uint32_t constexpr value() { return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::linear, MagnificationFilter::nearest, true>
{
    static uint32_t constexpr value() { return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::nearest_mipmap_nearest, MagnificationFilter::nearest, true>
{
    static uint32_t constexpr value() { return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::linear_mipmap_nearest, MagnificationFilter::nearest, true>
{
    static uint32_t constexpr value() { return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::nearest_mipmap_linear, MagnificationFilter::nearest, true>
{
    static uint32_t constexpr value() { return D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::linear_mipmap_linear, MagnificationFilter::nearest, true>
{
    static uint32_t constexpr value() { return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR; }
};



template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::nearest, MagnificationFilter::linear, true>
{
    static uint32_t constexpr value() { return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::linear, MagnificationFilter::linear, true>
{
    static uint32_t constexpr value() { return D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::nearest_mipmap_nearest, MagnificationFilter::linear, true>
{
    static uint32_t constexpr value() { return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::linear_mipmap_nearest, MagnificationFilter::linear, true>
{
    static uint32_t constexpr value() { return D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::nearest_mipmap_linear, MagnificationFilter::linear, true>
{
    static uint32_t constexpr value() { return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR; }
};

template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::linear_mipmap_linear, MagnificationFilter::linear, true>
{
    static uint32_t constexpr value() { return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR; }
};



template<>
struct MinMagFilterConverter<EngineAPI::Direct3D12, MinificationFilter::anisotropic, MagnificationFilter::anisotropic, true>
{
    static uint32_t constexpr value() { return D3D12_FILTER_COMPARISON_ANISOTROPIC; }
};

}}}


namespace lexgine { namespace core { namespace dx { namespace d3d12 {
//! Retruns D3D12_FILTER* enumeration constant based on provided runtime minification and magnification filter values
inline D3D12_FILTER d3d12Convert(MinificationFilter min_filter, MagnificationFilter mag_filter, bool is_comparison = false)
{
    if (!is_comparison)
    {
        switch (mag_filter)
        {
        case MagnificationFilter::nearest:
            switch (min_filter)
            {
            case MinificationFilter::nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::nearest, MagnificationFilter::nearest>::value());
            case MinificationFilter::linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::linear, MagnificationFilter::nearest>::value());
            case MinificationFilter::nearest_mipmap_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::nearest_mipmap_nearest, MagnificationFilter::nearest>::value());
            case MinificationFilter::linear_mipmap_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::linear_mipmap_nearest, MagnificationFilter::nearest>::value());
            case MinificationFilter::nearest_mipmap_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::nearest_mipmap_linear, MagnificationFilter::nearest>::value());
            case MinificationFilter::linear_mipmap_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::linear_mipmap_linear, MagnificationFilter::nearest>::value());
            default: throw;    // throw on unsupported combination of minification and magnification filters
            }
            break;

        case MagnificationFilter::linear:
            switch (min_filter)
            {
            case MinificationFilter::nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::nearest, MagnificationFilter::linear>::value());
            case MinificationFilter::linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::linear, MagnificationFilter::linear>::value());
            case MinificationFilter::nearest_mipmap_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::nearest_mipmap_nearest, MagnificationFilter::linear>::value());
            case MinificationFilter::linear_mipmap_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::linear_mipmap_nearest, MagnificationFilter::linear>::value());
            case MinificationFilter::nearest_mipmap_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::nearest_mipmap_linear, MagnificationFilter::linear>::value());
            case MinificationFilter::linear_mipmap_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::linear_mipmap_linear, MagnificationFilter::linear>::value());
            default: throw;    // throw on unsupported combination of minification and magnification filters
            }
            break;



        case MagnificationFilter::minimum_linear:
            switch (min_filter)
            {
            case MinificationFilter::minimum_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::minimum_nearest, MagnificationFilter::minimum_linear>::value());
            case MinificationFilter::minimum_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::minimum_linear, MagnificationFilter::minimum_linear>::value());
            case MinificationFilter::minimum_nearest_mipmap_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::minimum_nearest_mipmap_nearest, MagnificationFilter::minimum_linear>::value());
            case MinificationFilter::minimum_linear_mipmap_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::minimum_linear_mipmap_nearest, MagnificationFilter::minimum_linear>::value());
            case MinificationFilter::minimum_nearest_mipmap_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::minimum_nearest_mipmap_linear, MagnificationFilter::minimum_linear>::value());
            case MinificationFilter::minimum_linear_mipmap_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::minimum_linear_mipmap_linear, MagnificationFilter::minimum_linear>::value());
            default: throw;    // throw on unsupported combination of minification and magnification filters
            }
            break;

        case MagnificationFilter::minimum_nearest:
            switch (min_filter)
            {
            case MinificationFilter::minimum_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::minimum_nearest, MagnificationFilter::minimum_nearest>::value());
            case MinificationFilter::minimum_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::minimum_linear, MagnificationFilter::minimum_nearest>::value());
            case MinificationFilter::minimum_nearest_mipmap_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::minimum_nearest_mipmap_nearest, MagnificationFilter::minimum_nearest>::value());
            case MinificationFilter::minimum_linear_mipmap_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::minimum_linear_mipmap_nearest, MagnificationFilter::minimum_nearest>::value());
            case MinificationFilter::minimum_nearest_mipmap_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::minimum_nearest_mipmap_linear, MagnificationFilter::minimum_nearest>::value());
            case MinificationFilter::minimum_linear_mipmap_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::minimum_linear_mipmap_linear, MagnificationFilter::minimum_nearest>::value());
            default: throw;    // throw on unsupported combination of minification and magnification filters
            }
            break;



        case MagnificationFilter::maximum_linear:
            switch (min_filter)
            {
            case MinificationFilter::maximum_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::maximum_nearest, MagnificationFilter::maximum_linear>::value());
            case MinificationFilter::maximum_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::maximum_linear, MagnificationFilter::maximum_linear>::value());
            case MinificationFilter::maximum_nearest_mipmap_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::maximum_nearest_mipmap_nearest, MagnificationFilter::maximum_linear>::value());
            case MinificationFilter::maximum_linear_mipmap_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::maximum_linear_mipmap_nearest, MagnificationFilter::maximum_linear>::value());
            case MinificationFilter::maximum_nearest_mipmap_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::maximum_nearest_mipmap_linear, MagnificationFilter::maximum_linear>::value());
            case MinificationFilter::maximum_linear_mipmap_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::maximum_linear_mipmap_linear, MagnificationFilter::maximum_linear>::value());
            default: throw;    // throw on unsupported combination of minification and magnification filters
            }
            break;

        case MagnificationFilter::maximum_nearest:
            switch (min_filter)
            {
            case MinificationFilter::maximum_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::maximum_nearest, MagnificationFilter::maximum_nearest>::value());
            case MinificationFilter::maximum_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::maximum_linear, MagnificationFilter::maximum_nearest>::value());
            case MinificationFilter::maximum_nearest_mipmap_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::maximum_nearest_mipmap_nearest, MagnificationFilter::maximum_nearest>::value());
            case MinificationFilter::maximum_linear_mipmap_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::maximum_linear_mipmap_nearest, MagnificationFilter::maximum_nearest>::value());
            case MinificationFilter::maximum_nearest_mipmap_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::maximum_nearest_mipmap_linear, MagnificationFilter::maximum_nearest>::value());
            case MinificationFilter::maximum_linear_mipmap_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::maximum_linear_mipmap_linear, MagnificationFilter::maximum_nearest>::value());
            default: throw;    // throw on unsupported combination of minification and magnification filters
            }
            break;



        case MagnificationFilter::anisotropic:
            if (min_filter == MinificationFilter::anisotropic)
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::anisotropic, MagnificationFilter::anisotropic>::value());
            else throw;    // throw on unsupported combination of minification and magnification filters
            break;

        case MagnificationFilter::minimum_anisotropic:
            if (min_filter == MinificationFilter::minimum_anisotropic)
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::minimum_anisotropic, MagnificationFilter::minimum_anisotropic>::value());
            else throw;    // throw on unsupported combination of minification and magnification filters
            break;

        case MagnificationFilter::maximum_anisotropic:
            if (min_filter == MinificationFilter::maximum_anisotropic)
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::maximum_anisotropic, MagnificationFilter::maximum_anisotropic>::value());
            else throw;    // throw on unsupported combination of minification and magnification filters
            break;


        default: throw;    // throw on unsupported combination of minification and magnification filters
        }
    }
    else
    {
        switch (mag_filter)
        {
        case MagnificationFilter::nearest:
            switch (min_filter)
            {
            case MinificationFilter::nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::nearest, MagnificationFilter::nearest, true>::value());
            case MinificationFilter::linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::linear, MagnificationFilter::nearest, true>::value());
            case MinificationFilter::nearest_mipmap_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::nearest_mipmap_nearest, MagnificationFilter::nearest, true>::value());
            case MinificationFilter::linear_mipmap_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::linear_mipmap_nearest, MagnificationFilter::nearest, true>::value());
            case MinificationFilter::nearest_mipmap_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::nearest_mipmap_linear, MagnificationFilter::nearest, true>::value());
            case MinificationFilter::linear_mipmap_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::linear_mipmap_linear, MagnificationFilter::nearest, true>::value());
            default: throw;    // throw on unsupported combination of minification and magnification filters
            }
            break;

        case MagnificationFilter::linear:
            switch (min_filter)
            {
            case MinificationFilter::nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::nearest, MagnificationFilter::linear, true>::value());
            case MinificationFilter::linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::linear, MagnificationFilter::linear, true>::value());
            case MinificationFilter::nearest_mipmap_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::nearest_mipmap_nearest, MagnificationFilter::linear, true>::value());
            case MinificationFilter::linear_mipmap_nearest:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::linear_mipmap_nearest, MagnificationFilter::linear, true>::value());
            case MinificationFilter::nearest_mipmap_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::nearest_mipmap_linear, MagnificationFilter::linear, true>::value());
            case MinificationFilter::linear_mipmap_linear:
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::linear_mipmap_linear, MagnificationFilter::linear, true>::value());
            default: throw;    // throw on unsupported combination of minification and magnification filters
            }
            break;

        case MagnificationFilter::anisotropic:
            if (min_filter == MinificationFilter::anisotropic)
                return static_cast<D3D12_FILTER>(misc::MinMagFilterConverter<misc::EngineAPI::Direct3D12, MinificationFilter::anisotropic, MagnificationFilter::anisotropic, true>::value());
            else throw;    // throw on unsupported combination of minification and magnification filters
            break;

        default: throw;    // throw on unsupported combination of minification and magnification filters
        }
    }
}

}}}}

#define LEXGINE_CORE_DX_D3D12_MIN_MAG_FILTER_CONVERTER_INL
#endif
