#ifndef LEXGINE_SCENEGRAPH_SAMPLER_H
#define LEXGINE_SCENEGRAPH_SAMPLER_H

#include <engine/core/entity.h>
#include <engine/scenegraph/class_names.h>

namespace lexgine::scenegraph
{

enum class MinificationFilter
{
    nearest,
    linear,
    nearest_mipmap_nearest,
    linear_mipmap_nearest,
    nearest_mipmap_linear,
    linear_mipmap_linear
};

enum class MagnificationFilter {
    nearest,
    linear
};

enum class WrapMode {
    repeat,
    mirrored_repeat,
    clamp_to_edge
};

struct Sampler final : core::NamedEntity<class_names::Sampler>
{
    MinificationFilter min_filter;
    MagnificationFilter mag_filter;
    WrapMode wrap_s;
    WrapMode wrap_t;

    Sampler()
        : min_filter { MinificationFilter::linear_mipmap_linear }
        , mag_filter { MagnificationFilter::linear }
        , wrap_s { WrapMode::repeat }
        , wrap_t { WrapMode::repeat }
    {
    }

    Sampler(MinificationFilter min_filter, MagnificationFilter mag_filter, WrapMode wrap_s, WrapMode wrap_t)
        : min_filter { min_filter }
        , mag_filter { mag_filter }
        , wrap_s { wrap_s }
        , wrap_t { wrap_t }
    {
    }
};

}

#endif
