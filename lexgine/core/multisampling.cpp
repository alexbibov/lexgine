#include "multisampling.h"

using namespace lexgine::core;

MultiSamplingFormat::MultiSamplingFormat(uint32_t sample_count, uint32_t ms_quality):
    count{ sample_count },
    quality{ ms_quality }
{

}
