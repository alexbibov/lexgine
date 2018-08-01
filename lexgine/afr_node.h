#ifndef LEXGINE_CORE_AFR_NODE_H
#define LEXGINE_CORE_AFR_NODE_H

#include "lexgine/core/misc/static_vector.h"

namespace lexgine { namespace core {

template<typename T>
class AfrNode
{
public:
    AfrNode(uint32_t m_num_active_nodes = 1U);

    T& get(uint32_t index);
    T const& get(uint32_t index) const;

    void set(uint32_t index, T const&)

private:
    uint32_t m_num_active_nodes;
    misc::StaticVector<T, 32> m_resources;
};

}}

#endif
