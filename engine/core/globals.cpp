#include "globals.h"
#include "global_settings.h"
#include "gpu_data_blob_cache.h"
#include "engine/conversion/image_loader_pool.h"
#include "engine/conversion/texture_converter.h"
#include "engine/core/dx/d3d12/device.h"
#include "engine/core/dx/d3d12/dx_resource_factory.h"
#include "engine/core/dx/d3d12/caches/hlsl_shader_blob_cache.h"
#include "engine/core/dx/d3d12/caches/pso_blob_cache.h"
#include "engine/core/dx/d3d12/caches/root_signature_blob_cache.h"


using namespace lexgine::core;


Globals::Globals(EngineApi engine_api)
    : m_engine_api{ engine_api }
{
}

Globals::~Globals() = default;

void Globals::setGlobalSettings(std::unique_ptr<GlobalSettings> global_settings)
{
    m_global_settings = std::move(global_settings);
}

void Globals::setDxResourceFactory(std::unique_ptr<dx::d3d12::DxResourceFactory> dx_resource_factory)
{
    m_dx_resource_factory = std::move(dx_resource_factory);
}

void Globals::setGpuDataBlobCache(std::unique_ptr<GpuDataBlobCache> gpu_data_blob_cache)
{
    m_gpu_data_blob_cache = std::move(gpu_data_blob_cache);
}

void Globals::setDevice(dx::d3d12::Device& device)
{
    m_device = &device;
}

void Globals::setTextureConverter(std::unique_ptr<conversion::TextureConverter> texture_converter)
{
    m_texture_converter = std::move(texture_converter);
}

void Globals::setImageLoaderPool(std::unique_ptr<conversion::ImageLoaderPool> image_loader_pool)
{
    m_image_loader_pool = std::move(image_loader_pool);
}

void Globals::setHlslShaderBlobCache(std::unique_ptr<dx::d3d12::caches::HLSLShaderBlobCache> hlsl_shader_blob_cache)
{
    m_hlsl_shader_blob_cache = std::move(hlsl_shader_blob_cache);
}

void Globals::setRootSignatureBlobCache(std::unique_ptr<dx::d3d12::caches::RootSignatureBlobCache> root_signature_blob_cache)
{
    m_root_signature_blob_cache = std::move(root_signature_blob_cache);
}

void Globals::setPsoBlobCache(std::unique_ptr<dx::d3d12::caches::PSOBlobCache> pso_blob_cache)
{
    m_pso_blob_cache = std::move(pso_blob_cache);
}

void Globals::resetDeviceDependentCaches()
{
    m_pso_blob_cache.reset();
    m_root_signature_blob_cache.reset();
    m_hlsl_shader_blob_cache.reset();
}

void GlobalsAttorney<dx::D3D12Initializer>::setGlobalSettings(Globals& globals,
    std::unique_ptr<GlobalSettings> global_settings)
{
    globals.setGlobalSettings(std::move(global_settings));
}

void GlobalsAttorney<dx::D3D12Initializer>::setDxResourceFactory(Globals& globals,
    std::unique_ptr<dx::d3d12::DxResourceFactory> dx_resource_factory)
{
    globals.setDxResourceFactory(std::move(dx_resource_factory));
}

void GlobalsAttorney<dx::D3D12Initializer>::setGpuDataBlobCache(Globals& globals,
    std::unique_ptr<GpuDataBlobCache> gpu_data_blob_cache)
{
    globals.setGpuDataBlobCache(std::move(gpu_data_blob_cache));
}

void GlobalsAttorney<dx::D3D12Initializer>::setDevice(Globals& globals, dx::d3d12::Device& device)
{
    globals.setDevice(device);
}

void GlobalsAttorney<dx::D3D12Initializer>::setTextureConverter(Globals& globals,
    std::unique_ptr<conversion::TextureConverter> texture_converter)
{
    globals.setTextureConverter(std::move(texture_converter));
}

void GlobalsAttorney<dx::D3D12Initializer>::setHlslShaderBlobCache(Globals& globals,
    std::unique_ptr<dx::d3d12::caches::HLSLShaderBlobCache> hlsl_shader_blob_cache)
{
    globals.setHlslShaderBlobCache(std::move(hlsl_shader_blob_cache));
}

void GlobalsAttorney<dx::D3D12Initializer>::setRootSignatureBlobCache(Globals& globals,
    std::unique_ptr<dx::d3d12::caches::RootSignatureBlobCache> root_signature_blob_cache)
{
    globals.setRootSignatureBlobCache(std::move(root_signature_blob_cache));
}

void GlobalsAttorney<dx::D3D12Initializer>::setPsoBlobCache(Globals& globals,
    std::unique_ptr<dx::d3d12::caches::PSOBlobCache> pso_blob_cache)
{
    globals.setPsoBlobCache(std::move(pso_blob_cache));
}

void GlobalsAttorney<dx::D3D12Initializer>::resetDeviceDependentCaches(Globals& globals)
{
    globals.resetDeviceDependentCaches();
}

void GlobalsAttorney<lexgine::Initializer>::setImageLoaderPool(Globals& globals,
    std::unique_ptr<conversion::ImageLoaderPool> image_loader_pool)
{
    globals.setImageLoaderPool(std::move(image_loader_pool));
}
