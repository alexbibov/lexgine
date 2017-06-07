#ifndef LEXGINE_CORE_MULTISAMPLING_H

#include <cstdint>

namespace lexgine {namespace core {

//! API- and OS- agnostic description of multi-sampling format
struct MultiSamplingFormat
{
    uint32_t count;    //!< number of samples
    uint32_t quality;    //!< quality of multi-sampling. The exact meaning is OS- API- and video driver dependent

    MultiSamplingFormat() = default;
    MultiSamplingFormat(uint32_t sample_count, uint32_t ms_quality);
};


}}

#define LEXGINE_CORE_MULTISAMPLING_H
#endif
