#ifndef LEXGINE_CORE_PRIMITIVE_TOPOLOGY_H
#define  LEXGINE_CORE_PRIMITIVE_TOPOLOGY_H

#include <cstdint>

namespace lexgine::core {

//! API-agnostic primitive topology type
enum class PrimitiveTopologyType: uint8_t
{
    point,
    line,
    triangle,
    patch
};

//! API-agnostic primitive topology
enum class PrimitiveTopology : uint8_t
{
    undefined, 
    point_list,
    line_list,
    line_strip,
    triangle_list,
    triangle_strip,

    line_list_adjacent,
    line_strip_adjacent,
    triangle_list_adjacent,
    triangle_strip_adjacent,

    patch_list_1_ctrl_point,
    patch_list_2_ctrl_points,
    patch_list_3_ctrl_points,
    patch_list_4_ctrl_points,
    patch_list_5_ctrl_points,
    patch_list_6_ctrl_points,
    patch_list_7_ctrl_points,
    patch_list_8_ctrl_points,
    patch_list_9_ctrl_points,
    patch_list_10_ctrl_points,
    patch_list_11_ctrl_points,
    patch_list_12_ctrl_points,
    patch_list_13_ctrl_points,
    patch_list_14_ctrl_points,
    patch_list_15_ctrl_points,
    patch_list_16_ctrl_points,
    patch_list_17_ctrl_points,
    patch_list_18_ctrl_points,
    patch_list_19_ctrl_points,
    patch_list_20_ctrl_points,
    patch_list_21_ctrl_points,
    patch_list_22_ctrl_points,
    patch_list_23_ctrl_points,
    patch_list_24_ctrl_points,
    patch_list_25_ctrl_points,
    patch_list_26_ctrl_points,
    patch_list_27_ctrl_points,
    patch_list_28_ctrl_points,
    patch_list_29_ctrl_points,
    patch_list_30_ctrl_points,
    patch_list_31_ctrl_points,
    patch_list_32_ctrl_points
};

}

#endif