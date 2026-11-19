#ifndef LEXGINE_CORE_GLOBALS_H
#define LEXGINE_CORE_GLOBALS_H

#include <memory>

#include <engine/core/entity.h>
#include <engine/core/engine_api.h>
#include <engine/core/lexgine_core_fwd.h>
#include <engine/core/dx/d3d12_globals.h>
#include <engine/conversion/lexgine_conversion_fwd.h>


namespace lexgine {

class Initializer;

}

namespace lexgine::core {

template<typename T> class GlobalsAttorney;

//! Registry of shared engine services and the globals for the selected graphics API.
class Globals : NamedEntity<Globals>
{
    friend class GlobalsAttorney<dx::D3D12Initializer>;
    friend class GlobalsAttorney<lexgine::Initializer>;

public:
    explicit Globals(EngineApi engine_api);
    ~Globals();

    Globals(Globals const&) = delete;
    Globals& operator=(Globals const&) = delete;

    //! Returns the graphics API backing the engine
    EngineApi engineApi() const { return m_engine_api; }

    //! Available when engineApi() is Direct3D12
    dx::D3D12Globals& d3d12() { return *m_d3d12_globals; }
    dx::D3D12Globals const& d3d12() const { return *m_d3d12_globals; }

    //! Not synchronized: reached while the engine is initialized and while a scene is loaded
    conversion::ImageLoaderPool& imageLoaderPool() { return *m_image_loader_pool; }
    conversion::ImageLoaderPool const& imageLoaderPool() const { return *m_image_loader_pool; }

    //! Registry of all textures available for debug views
    DebugTexturesRegistry& debugTextureRegistry() { return *m_debug_textures_registry; }
    DebugTexturesRegistry const& debugTextureRegistry() const { return *m_debug_textures_registry; }

private:
    EngineApi m_engine_api;

    std::unique_ptr<conversion::ImageLoaderPool> m_image_loader_pool;
    std::unique_ptr<dx::D3D12Globals> m_d3d12_globals;
    std::unique_ptr<DebugTexturesRegistry> m_debug_textures_registry;
};


template<> class GlobalsAttorney<dx::D3D12Initializer>
{
    friend class dx::D3D12Initializer;

    static void setD3D12Globals(Globals& globals, std::unique_ptr<dx::D3D12Globals> d3d12_globals);
};


template<> class GlobalsAttorney<lexgine::Initializer>
{
    friend class lexgine::Initializer;

    static void setImageLoaderPool(Globals& globals, std::unique_ptr<conversion::ImageLoaderPool> image_loader_pool);
    static void setDebugTexturesRegistry(Globals& globals, std::unique_ptr<DebugTexturesRegistry> debug_textures_registry);
};

}

#endif
