#ifndef LEXGINE_CORE_GLOBAL_CONSTANTS_GLOBAL_CONSTANTS_H
#define LEXGINE_CORE_GLOBAL_CONSTANTS_GLOBAL_CONSTANTS_H

#include "engine/core/streamed_cache.h"

namespace lexgine::core::global_constants {

static size_t constexpr combined_cache_cluster_size = 256U;
static StreamedCacheCompressionLevel constexpr combined_cache_compression_level = StreamedCacheCompressionLevel::level3;
static char const* combined_cache_extra_extension = "thread";

}

#endif
