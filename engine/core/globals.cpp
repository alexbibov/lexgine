#include "globals.h"
#include "engine/core/debug_textures_registry.h"
#include "engine/conversion/image_loader_pool.h"


using namespace lexgine::core;


Globals::Globals(EngineApi engine_api)
    : m_engine_api{ engine_api }
{
}

Globals::~Globals() = default;

void GlobalsAttorney<dx::D3D12Initializer>::setD3D12Globals(Globals& globals,
    std::unique_ptr<dx::D3D12Globals> d3d12_globals)
{
    globals.m_d3d12_globals = std::move(d3d12_globals);
}

void GlobalsAttorney<lexgine::Initializer>::setImageLoaderPool(Globals& globals,
    std::unique_ptr<conversion::ImageLoaderPool> image_loader_pool)
{
    globals.m_image_loader_pool = std::move(image_loader_pool);
}

void GlobalsAttorney<lexgine::Initializer>::setDebugTexturesRegistry(Globals& globals,
    std::unique_ptr<DebugTexturesRegistry> debug_textures_registry)
{
    globals.m_debug_textures_registry = std::move(debug_textures_registry);
}
