#ifndef LEXGINE_CORE_DX_D3D12_TASKS_ROOT_SIGNATURE_BUILDER_H
#define LEXGINE_CORE_DX_D3D12_TASKS_ROOT_SIGNATURE_BUILDER_H

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/root_signature.h"
#include "engine/core/dx/d3d12/task_caches/combined_cache_key.h"

namespace lexgine::core::dx::d3d12::tasks {

class RootSignatureBuilder
{
public:
    RootSignatureBuilder(task_caches::CombinedCacheKey const& key,
        Globals& globals, RootSignature&& root_signature, RootSignatureFlags const& flags,
        misc::DateTime const& timestamp);

    bool build(uint8_t worker_id);
    D3DDataBlob const& getTaskData() const;
    bool wasSuccessful() const;

    /*! returns string name associated with the root signature in root signature blob cache
        The names are required to follow special convention (note the '__ROOTSIGNATURE' suffix):

            <user_defined_string_name><user_defined_numeric_id>__ROOTSIGNATURE

        For example: forward_illumination_unshadowed_pass00__ROOTSIGNATURE
    */
    std::string getCacheName() const;

private:
    task_caches::CombinedCacheKey const& m_key;
    Globals& m_globals;
    RootSignature m_rs;
    RootSignatureFlags m_rs_flags;
    bool m_was_successful;
    D3DDataBlob m_compiled_rs_blob;
    misc::DateTime m_timestamp;
};

}

#endif
