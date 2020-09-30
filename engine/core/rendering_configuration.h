#ifndef LEXGINE_CORE_RENDERING_CONFIGURATION_H
#define LEXGINE_CORE_RENDERING_CONFIGURATION_H

#include "viewport.h"
#include "engine/osinteraction/windows/window.h"
#include "engine/core/dx/dxgi/common.h"

namespace lexgine::core {

struct RenderingConfiguration
{
    Viewport viewport;
    DXGI_FORMAT color_buffer_format;
    DXGI_FORMAT depth_buffer_format;
    osinteraction::windows::Window* p_rendering_window;
};

}

#endif
