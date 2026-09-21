#ifndef LEXGINE_CORE_GLOBALS_H
#define LEXGINE_CORE_GLOBALS_H

#include <memory>

#include <engine/core/entity.h>
#include <engine/core/engine_api.h>
#include <engine/core/lexgine_core_fwd.h>
#include <engine/core/dx/lexgine_core_dx_fwd.h>
#include <engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h>
#include <engine/core/dx/d3d12/caches/lexgine_core_dx_d3d12_caches_fwd.h>
#include <engine/conversion/lexgine_conversion_fwd.h>


namespace lexgine {

class Initializer;

}

namespace lexgine::core {

template<typename T> class GlobalsAttorney;

/*! Registry of the engine-wide objects with a fixed, statically known set of entries.

 Every entry is declared explicitly, so the contents of the registry are described by the type system
 rather than discovered at runtime. Read access is available to any holder of a Globals reference, while
 population of the registry is restricted to the engine initializers through GlobalsAttorney.

 Adding a new engine-wide object requires declaring a corresponding entry here.
*/
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

    GlobalSettings& globalSettings() { return *m_global_settings; }
    GlobalSettings const& globalSettings() const { return *m_global_settings; }

    dx::d3d12::DxResourceFactory& dxResourceFactory() { return *m_dx_resource_factory; }
    dx::d3d12::DxResourceFactory const& dxResourceFactory() const { return *m_dx_resource_factory; }

    GpuDataBlobCache& gpuDataBlobCache() { return *m_gpu_data_blob_cache; }
    GpuDataBlobCache const& gpuDataBlobCache() const { return *m_gpu_data_blob_cache; }

    //! Returns the device currently selected for graphics and compute work
    dx::d3d12::Device& device() { return *m_device; }
    dx::d3d12::Device const& device() const { return *m_device; }

    conversion::TextureConverter& textureConverter() { return *m_texture_converter; }
    conversion::TextureConverter const& textureConverter() const { return *m_texture_converter; }

    conversion::ImageLoaderPool& imageLoaderPool() { return *m_image_loader_pool; }
    conversion::ImageLoaderPool const& imageLoaderPool() const { return *m_image_loader_pool; }

    dx::d3d12::caches::HLSLShaderBlobCache& hlslShaderBlobCache() { return *m_hlsl_shader_blob_cache; }
    dx::d3d12::caches::HLSLShaderBlobCache const& hlslShaderBlobCache() const { return *m_hlsl_shader_blob_cache; }

    dx::d3d12::caches::RootSignatureBlobCache& rootSignatureBlobCache() { return *m_root_signature_blob_cache; }
    dx::d3d12::caches::RootSignatureBlobCache const& rootSignatureBlobCache() const { return *m_root_signature_blob_cache; }

    dx::d3d12::caches::PSOBlobCache& psoBlobCache() { return *m_pso_blob_cache; }
    dx::d3d12::caches::PSOBlobCache const& psoBlobCache() const { return *m_pso_blob_cache; }

private:
    void setGlobalSettings(std::unique_ptr<GlobalSettings> global_settings);
    void setDxResourceFactory(std::unique_ptr<dx::d3d12::DxResourceFactory> dx_resource_factory);
    void setGpuDataBlobCache(std::unique_ptr<GpuDataBlobCache> gpu_data_blob_cache);
    void setDevice(dx::d3d12::Device& device);
    void setTextureConverter(std::unique_ptr<conversion::TextureConverter> texture_converter);
    void setImageLoaderPool(std::unique_ptr<conversion::ImageLoaderPool> image_loader_pool);
    void setHlslShaderBlobCache(std::unique_ptr<dx::d3d12::caches::HLSLShaderBlobCache> hlsl_shader_blob_cache);
    void setRootSignatureBlobCache(std::unique_ptr<dx::d3d12::caches::RootSignatureBlobCache> root_signature_blob_cache);
    void setPsoBlobCache(std::unique_ptr<dx::d3d12::caches::PSOBlobCache> pso_blob_cache);

    void resetDeviceDependentCaches();

private:
    EngineApi m_engine_api;

    std::unique_ptr<GlobalSettings> m_global_settings;
    std::unique_ptr<dx::d3d12::DxResourceFactory> m_dx_resource_factory;
    std::unique_ptr<GpuDataBlobCache> m_gpu_data_blob_cache;
    std::unique_ptr<conversion::ImageLoaderPool> m_image_loader_pool;
    std::unique_ptr<conversion::TextureConverter> m_texture_converter;
    std::unique_ptr<dx::d3d12::caches::HLSLShaderBlobCache> m_hlsl_shader_blob_cache;
    std::unique_ptr<dx::d3d12::caches::RootSignatureBlobCache> m_root_signature_blob_cache;
    std::unique_ptr<dx::d3d12::caches::PSOBlobCache> m_pso_blob_cache;

    dx::d3d12::Device* m_device{ nullptr };
};


template<> class GlobalsAttorney<dx::D3D12Initializer>
{
    friend class dx::D3D12Initializer;

    static void setGlobalSettings(Globals& globals, std::unique_ptr<GlobalSettings> global_settings);
    static void setDxResourceFactory(Globals& globals, std::unique_ptr<dx::d3d12::DxResourceFactory> dx_resource_factory);
    static void setGpuDataBlobCache(Globals& globals, std::unique_ptr<GpuDataBlobCache> gpu_data_blob_cache);
    static void setDevice(Globals& globals, dx::d3d12::Device& device);
    static void setTextureConverter(Globals& globals, std::unique_ptr<conversion::TextureConverter> texture_converter);
    static void setHlslShaderBlobCache(Globals& globals,
        std::unique_ptr<dx::d3d12::caches::HLSLShaderBlobCache> hlsl_shader_blob_cache);
    static void setRootSignatureBlobCache(Globals& globals,
        std::unique_ptr<dx::d3d12::caches::RootSignatureBlobCache> root_signature_blob_cache);
    static void setPsoBlobCache(Globals& globals, std::unique_ptr<dx::d3d12::caches::PSOBlobCache> pso_blob_cache);

    static void resetDeviceDependentCaches(Globals& globals);
};


template<> class GlobalsAttorney<lexgine::Initializer>
{
    friend class lexgine::Initializer;

    static void setImageLoaderPool(Globals& globals, std::unique_ptr<conversion::ImageLoaderPool> image_loader_pool);
};

}

#endif
