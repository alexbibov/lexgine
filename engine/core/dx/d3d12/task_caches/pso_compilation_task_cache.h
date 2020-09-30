#ifndef LEXGINE_CORE_DX_D3D12_TASK_CACHES_PSO_COMPILATION_TASK_CACHE_H
#define LEXGINE_CORE_DX_D3D12_TASK_CACHES_PSO_COMPILATION_TASK_CACHE_H

#include <list>
#include <map>
#include <set>

#include "engine/core/entity.h"
#include "engine/core/class_names.h"
#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/pipeline_state.h"
#include "engine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"


namespace lexgine::core::dx::d3d12::task_caches {

class PSOCompilationTaskCache : public NamedEntity<class_names::D3D12_PSOCompilationTaskCache>
{
    friend class tasks::GraphicsPSOCompilationTask;
    friend class tasks::ComputePSOCompilationTask;
    friend class CombinedCacheKey;

public:

    using graphics_pso_cache_storage = std::list<tasks::GraphicsPSOCompilationTask>;
    using compute_pso_cache_storage = std::list<tasks::ComputePSOCompilationTask>;

    class VersionedPSODescriptor
    {
    public:
        VersionedPSODescriptor()
            : m_timestamp{ misc::DateTime::buildTime() }
        {

        }

        misc::DateTime const& timestamp() const { return m_timestamp; }

    private:
        misc::DateTime m_timestamp;
    };

    class VersionedGraphicsPSODescriptor final : public VersionedPSODescriptor
    {
    public:
        VersionedGraphicsPSODescriptor(GraphicsPSODescriptor const& descriptor)
            : m_descriptor{ descriptor }
        {

        }

        GraphicsPSODescriptor const& descriptor() const { return m_descriptor; }

    private:
        GraphicsPSODescriptor m_descriptor;
    };

    class VersionedComputePSODescriptor final : public VersionedPSODescriptor
    {
    public:
        VersionedComputePSODescriptor(ComputePSODescriptor const& descriptor)
            : m_descriptor{ descriptor }
        {

        }

        ComputePSODescriptor const& descriptor() const { return m_descriptor; }

    private:
        ComputePSODescriptor m_descriptor;
    };

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

    tasks::GraphicsPSOCompilationTask* findOrCreateTask(
        Globals& globals,
        VersionedGraphicsPSODescriptor const& versioned_descriptor,
        std::string const& pso_cache_name, uint64_t uid);

    tasks::ComputePSOCompilationTask* findOrCreateTask(
        Globals& globals,
        VersionedComputePSODescriptor const& versioned_descriptor,
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
