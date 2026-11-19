#include "d3d12_globals.h"

#include "engine/conversion/texture_converter.h"
#include "engine/core/global_settings.h"
#include "engine/core/gpu_data_blob_cache.h"
#include "engine/core/dx/d3d12/dx_resource_factory.h"
#include "engine/core/dx/d3d12/caches/hlsl_shader_blob_cache.h"
#include "engine/core/dx/d3d12/caches/pso_blob_cache.h"
#include "engine/core/dx/d3d12/caches/root_signature_blob_cache.h"

namespace lexgine::core::dx {

D3D12Globals::D3D12Globals() = default;
D3D12Globals::~D3D12Globals() = default;

void D3D12Globals::resetDeviceDependentCaches()
{
    m_pso_blob_cache.reset();
    m_root_signature_blob_cache.reset();
    m_hlsl_shader_blob_cache.reset();
}

void D3D12Globals::releaseDeviceDependentEntries()
{
    resetDeviceDependentCaches();
    m_texture_converter.reset();
    m_device = nullptr;
    m_dx_resource_factory.reset();
}

}
