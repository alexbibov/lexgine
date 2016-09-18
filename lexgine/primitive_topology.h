#ifndef LEXGINE_CORE_PRIMITIVE_TOPOLOGY_H

#include <cstdint>

namespace lexgine {
namespace core {

//! API-agnostic primitive topology
enum class PrimitiveTopology: uint8_t
{
    point,
    line,
    triangle,
    patch
};

}}


#define  LEXGINE_CORE_PRIMITIVE_TOPOLOGY
#endif