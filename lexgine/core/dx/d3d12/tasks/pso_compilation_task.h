#ifndef LEXGINE_CORE_DX_D3D12_TASKS_PSO_COMPILATION_TASK_H

#include "lexgine/core/concurrency/schedulable_task.h"
#include "lexgine/core/dx/d3d12/pipeline_state.h"
#include "lexgine/core/data_blob.h"
#include "lexgine/core/misc/static_vector.h"

#include "hlsl_compilation_task.h"

#include <list>

namespace lexgine {namespace core {namespace dx {namespace d3d12 {namespace tasks {


//! Implements compilation of provided HLSL source code
class PSOCompilationTask : public concurrency::SchedulableTask
{
public:
    class CacheKey
    {
        static constexpr size_t max_pso_cache_name_length_in_bytes = 512U;

        char cache_name[max_pso_cache_name_length_in_bytes];
        uint64_t uid;

        static size_t const serialize_size = max_pso_cache_name_length_in_bytes + sizeof(uint64_t);


        std::string toString() const;
        void serialize(void* p_serialization_blob) const;
        void deserialize(void const* p_serialization_blob);

        CacheKey() = default;
        CacheKey(std::string const& name, uint64_t uid);

        bool operator<(CacheKey const& other) const;
        bool operator==(CacheKey const& other) const;
    };

public:
    PSOCompilationTask(GraphicsPSODescriptor const& graphics_pso_desc, 
        std::string const& pso_cache_name, uint64_t uid);

    PSOCompilationTask(ComputePSODescriptor const& compute_pso_desc, 
        std::string const& pso_cache_name, uint64_t uid);

    D3DDataBlob getTaskData() const;

    //! Returns 'true' if PSO compilation has completed successfully; returns 'false' otherwise
    bool wasSuccessful() const;

    //! Executes the task manually. THROWS if PSO compilation fails.
    bool execute(uint8_t worker_id);

private:
    union PSODescriptor
    {
        GraphicsPSODescriptor graphics;
        ComputePSODescriptor compute;
    } m_pso_descriptor;

    enum class PSOType
    {
        graphics,
        compute
    } m_pso_type;

private:
    bool do_task(uint8_t worker_id, uint16_t frame_index) override;    //! performs actual compilation of the shader
    concurrency::TaskType get_task_type() const override;    //! returns type of this task (CPU)
};

}}}}}

#define LEXGINE_CORE_DX_D3D12_TASKS_PSO_COMPILATION_TASK_H
#endif
