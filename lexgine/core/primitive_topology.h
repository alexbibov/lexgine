#ifndef LEXGINE_CORE_PRIMITIVE_TOPOLOGY_H
#define  LEXGINE_CORE_PRIMITIVE_TOPOLOGY_H

#include <cstdint>

namespace lexgine::core {

//! API-agnostic primitive topology
enum class PrimitiveTopology: uint8_t
{
    point,
    line,
    triangle,
    patch
};

}

#endif