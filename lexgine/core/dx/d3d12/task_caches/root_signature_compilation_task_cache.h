#ifndef LEXGINE_CORE_DX_D3D12_TASK_CACHES_ROOT_SIGNATURE_COMPILATION_TASK_CACHE_H
#define LEXGINE_CORE_DX_D3D12_TASK_CACHES_ROOT_SIGNATURE_COMPILATION_TASK_CACHE_H

#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"
#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"
#include "lexgine/core/dx/d3d12/root_signature.h"

#include <list>
#include <map>

namespace lexgine { namespace core { namespace dx { namespace d3d12 { namespace task_caches {

class RootSignatureCompilationTaskCache : public NamedEntity<class_names::RootSignatureCompilationTaskCache>
{
    friend class tasks::RootSignatureCompilationTask;
    friend class CombinedCacheKey;

public:
    using cache_storage = std::list<tasks::RootSignatureCompilationTask>;

private:

    struct Key final
    {
        static constexpr size_t max_rs_cache_name_length = 512U;

        char rs_cache_name[max_rs_cache_name_length];
        uint64_t uid;

        static constexpr size_t const serialized_size = max_rs_cache_name_length + sizeof(uint64_t);

        std::string toString() const;

        void serialize(void* p_serialization_blob) const;
        void deserialize(void const* p_serialization_blob);

        Key(std::string const& root_signature_cache_name, uint64_t uid);
        Key() = default;

        bool operator<(Key const& other) const;
        bool operator==(Key const& other) const;
    };

    using cache_mapping = std::map<CombinedCacheKey, cache_storage::iterator>;

public:
    tasks::RootSignatureCompilationTask* addTask(
        Globals const& globals,
        RootSignature&& root_signature, RootSignatureFlags const& flags,
        std::string const& root_signature_cache_name, uint64_t uid);
    
    cache_storage& storage();

    cache_storage const& storage() const;

private:
    cache_storage m_rs_compilation_tasks;
    cache_mapping m_task_keys;

};

}}}}}

#endif
