#ifndef LEXGINE_CORE_COMMON_TYPES_H
#define LEXGINE_CORE_COMMON_TYPES_H

namespace lexgine::core {

enum class DescriptorHeapType {
    cbv_srv_uav,
    sampler,
    rtv,
    dsv,
    count
};

}


#endif

