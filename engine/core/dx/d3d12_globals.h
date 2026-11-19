#ifndef LEXGINE_CORE_DX_D3D12_GLOBALS_H
#define LEXGINE_CORE_DX_D3D12_GLOBALS_H

#include <memory>

#include "engine/conversion/lexgine_conversion_fwd.h"
#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/dx/lexgine_core_dx_fwd.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/caches/lexgine_core_dx_d3d12_caches_fwd.h"

namespace lexgine::core::dx {

class D3D12Globals
{
    friend class D3D12Initializer;

public:
    D3D12Globals();
    ~D3D12Globals();

    D3D12Globals(D3D12Globals const&) = delete;
    D3D12Globals& operator=(D3D12Globals const&) = delete;

    //! Immutable once the engine is initialized
    GlobalSettings& globalSettings() { return *m_global_settings; }
    GlobalSettings const& globalSettings() const { return *m_global_settings; }

    //! Not synchronized: its upload heap partitioning is only mutated at initialization and at scene load
    d3d12::DxResourceFactory& dxResourceFactory() { return *m_dx_resource_factory; }
    d3d12::DxResourceFactory const& dxResourceFactory() const { return *m_dx_resource_factory; }

    GpuDataBlobCache& gpuDataBlobCache() { return *m_gpu_data_blob_cache; }
    GpuDataBlobCache const& gpuDataBlobCache() const { return *m_gpu_data_blob_cache; }

    /*! Returns the device currently selected for graphics and compute work.

     Its descriptor allocators reserve through atomic counters and its bindless descriptor cache
     synchronizes itself, so the device is reachable from every thread recording the rendering work. Rotating
     the page of the transient allocator belongs to the master worker alone and asserts as much.
    */
    d3d12::Device& device() { return *m_device; }
    d3d12::Device const& device() const { return *m_device; }

    //! Not synchronized: reached while the engine is initialized and while a scene is loaded
    conversion::TextureConverter& textureConverter() { return *m_texture_converter; }
    conversion::TextureConverter const& textureConverter() const { return *m_texture_converter; }

    //! Synchronizes itself; reached from every thread recording the rendering work
    d3d12::caches::HLSLShaderBlobCache& hlslShaderBlobCache() { return *m_hlsl_shader_blob_cache; }
    d3d12::caches::HLSLShaderBlobCache const& hlslShaderBlobCache() const { return *m_hlsl_shader_blob_cache; }

    //! Synchronizes itself; reached from every thread recording the rendering work
    d3d12::caches::RootSignatureBlobCache& rootSignatureBlobCache() { return *m_root_signature_blob_cache; }
    d3d12::caches::RootSignatureBlobCache const& rootSignatureBlobCache() const { return *m_root_signature_blob_cache; }

    //! Synchronizes itself; reached from every thread recording the rendering work
    d3d12::caches::PSOBlobCache& psoBlobCache() { return *m_pso_blob_cache; }
    d3d12::caches::PSOBlobCache const& psoBlobCache() const { return *m_pso_blob_cache; }

private:
    void resetDeviceDependentCaches();
    void releaseDeviceDependentEntries();

private:
    std::unique_ptr<GlobalSettings> m_global_settings;
    std::unique_ptr<d3d12::DxResourceFactory> m_dx_resource_factory;
    std::unique_ptr<GpuDataBlobCache> m_gpu_data_blob_cache;
    std::unique_ptr<conversion::TextureConverter> m_texture_converter;
    std::unique_ptr<d3d12::caches::HLSLShaderBlobCache> m_hlsl_shader_blob_cache;
    std::unique_ptr<d3d12::caches::RootSignatureBlobCache> m_root_signature_blob_cache;
    std::unique_ptr<d3d12::caches::PSOBlobCache> m_pso_blob_cache;

    d3d12::Device* m_device{ nullptr };
};

}

#endif
