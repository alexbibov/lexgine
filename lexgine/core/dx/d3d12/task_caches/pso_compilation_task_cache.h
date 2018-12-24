#ifndef LEXGINE_CORE_DX_D3D12_TASK_CACHES_PSO_COMPILATION_TASK_CACHE_H
#define LEXGINE_CORE_DX_D3D12_TASK_CACHES_PSO_COMPILATION_TASK_CACHE_H

#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"
#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/dx/d3d12/pipeline_state.h"
#include "lexgine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"

#include <list>
#include <map>
#include <set>


namespace lexgine::core::dx::d3d12::task_caches {

class PSOCompilationTaskCache : public NamedEntity<class_names::D3D12_PSOCompilationTaskCache>
{
    friend class tasks::GraphicsPSOCompilationTask;
    friend class tasks::ComputePSOCompilationTask;
    friend class CombinedCacheKey;

public:

    using graphics_pso_cache_storage = std::list<tasks::GraphicsPSOCompilationTask>;
    using compute_pso_cache_storage = std::list<tasks::ComputePSOCompilationTask>;

private:

    struct Key final
    {
        static constexpr size_t max_pso_cache_name_length = 512U;

        char pso_cache_name[max_pso_cache_name_length];
        uint64_t uid;
        PSOType pso_type;

        static constexpr size_t const serialized_size =
            max_pso_cache_name_length
            + sizeof(uint64_t)
            + sizeof(PSOType);

        std::string toString() const;

        void serialize(void* p_serialization_blob) const;
        void deserialize(void const* p_serialization_blob);

        Key(std::string const& pso_cache_name, uint64_t uid, PSOType pso_type);
        Key() = default;

        bool operator<(Key const& other) const;
        bool operator==(Key const& other) const;
    };

    using cache_mapping = std::map<CombinedCacheKey, 
        std::pair<graphics_pso_cache_storage::iterator, compute_pso_cache_storage::iterator>>;

public:

    tasks::GraphicsPSOCompilationTask* addTask(
        Globals& globals,
        GraphicsPSODescriptor const& descriptor,
        std::string const& pso_cache_name, uint64_t uid);

    tasks::ComputePSOCompilationTask* addTask(
        Globals& globals,
        ComputePSODescriptor const& descriptor,
        std::string const& pso_cache_name, uint64_t uid);

    graphics_pso_cache_storage& graphicsPSOStorage();
    graphics_pso_cache_storage const& graphicsPSOStorage() const;

    compute_pso_cache_storage& computePSOStorage();
    compute_pso_cache_storage const& computePSOStorage() const;



private:
    graphics_pso_cache_storage m_graphics_pso_compilation_tasks;
    compute_pso_cache_storage m_compute_pso_compilation_tasks;
    cache_mapping m_psos_cache_keys;
};

}

#endif
