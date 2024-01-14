#include "pso_compilation_task_cache.h"

#include <engine/core/dx/dxgi/hw_adapter_enumerator.h>
#include <engine/core/exception.h>
#include <engine/core/globals.h>
#include <engine/core/global_settings.h>
#include <engine/core/misc/strict_weak_ordering.h>
#include <engine/core/dx/d3d12/tasks/pso_compilation_task.h>
#include <engine/core/dx/d3d12/device.h>

#include "combined_cache_key.h"


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::task_caches;



std::string PSOCompilationTaskCache::Key::toString() const
{
    // Note: do not change, the naming is conventional

    return std::string{ pso_cache_name + std::to_string(uid)
        + (pso_type == PSOType::graphics ? "__GRAPHICSPSO" : "__COMPUTEPSO") };
}

void PSOCompilationTaskCache::Key::serialize(void* p_serialization_blob) const
{
    uint8_t* ptr = static_cast<uint8_t*>(p_serialization_blob);
    strcpy_s(reinterpret_cast<char*>(ptr), max_pso_cache_name_length, pso_cache_name); ptr += max_pso_cache_name_length;
    memcpy(ptr, &uid, sizeof(uint64_t)); ptr += sizeof(uint64_t);
    memcpy(ptr, &pso_type, sizeof(PSOType));
}

void PSOCompilationTaskCache::Key::deserialize(void const* p_serialization_blob)
{
    uint8_t const* ptr = static_cast<uint8_t const*>(p_serialization_blob);
    strcpy_s(pso_cache_name, max_pso_cache_name_length, reinterpret_cast<char const*>(ptr)); ptr += max_pso_cache_name_length;
    memcpy(&uid, ptr, sizeof(uid)); ptr += sizeof(uint64_t);
    memcpy(&pso_type, ptr, sizeof(PSOType));
}

PSOCompilationTaskCache::Key::Key(
    std::string const& pso_cache_name,
    uint64_t uid,
    PSOType pso_type) :
    uid{ uid },
    pso_type{ pso_type }
{
    strcpy_s(this->pso_cache_name, max_pso_cache_name_length, pso_cache_name.c_str());
}

bool PSOCompilationTaskCache::Key::operator<(Key const& other) const
{
    SWO_STEP(strcmp(pso_cache_name, other.pso_cache_name), < , 0);
    SWO_STEP(uid, < , other.uid);
    SWO_END(static_cast<unsigned char>(pso_type), < , static_cast<unsigned char>(other.pso_type));
}

bool PSOCompilationTaskCache::Key::operator==(Key const& other) const
{
    return strcmp(pso_cache_name, other.pso_cache_name) == 0
        && uid == other.uid
        && static_cast<unsigned char>(pso_type) == static_cast<unsigned char>(other.pso_type);
}


tasks::GraphicsPSOCompilationTask* PSOCompilationTaskCache::findOrCreateTask(
    Globals& globals,
    VersionedGraphicsPSODescriptor const& versioned_descriptor,
    std::string const& pso_cache_name, uint64_t uid)
{
    tasks::GraphicsPSOCompilationTask* new_pso_compilation_task{ nullptr };

    auto* active_adaptor_ptr = globals.get<Device>()->hwAdapter();
    auto adapter_luid = active_adaptor_ptr->getProperties().details.luid;
    std::string adapter_tailored_pso_cache_name = pso_cache_name + "__" + std::to_string(adapter_luid.LowPart) + std::to_string(adapter_luid.HighPart);


    Key key{ adapter_tailored_pso_cache_name, uid, PSOType::graphics };
    CombinedCacheKey combined_key{ key };

    auto q = m_psos_cache_keys.find(combined_key);
    if (q == m_psos_cache_keys.end())
    {
        auto psos_cache_keys_insertion_position =
            m_psos_cache_keys.insert(std::make_pair(
                combined_key, 
                std::make_pair(graphics_pso_cache_storage::iterator{}, compute_pso_cache_storage::iterator{}))).first;

        m_graphics_pso_compilation_tasks.emplace_back(
            psos_cache_keys_insertion_position->first, globals,
            versioned_descriptor.descriptor(), versioned_descriptor.timestamp()
        );

        graphics_pso_cache_storage::iterator p = --m_graphics_pso_compilation_tasks.end();
        psos_cache_keys_insertion_position->second.first = p;

        tasks::GraphicsPSOCompilationTask& new_graphics_pso_compilation_task_ref = *p;
        new_pso_compilation_task = &new_graphics_pso_compilation_task_ref;
        return new_pso_compilation_task;
    }
    else
    {
        return &(*q->second.first);
    }
}

tasks::ComputePSOCompilationTask* PSOCompilationTaskCache::findOrCreateTask(
    Globals& globals,
    VersionedComputePSODescriptor const& versioned_descriptor,
    std::string const& pso_cache_name, uint64_t uid)
{
    tasks::ComputePSOCompilationTask* new_pso_compilation_task{ nullptr };
    Key key{ pso_cache_name, uid, PSOType::compute };
    CombinedCacheKey combined_key{ key };

    auto q = m_psos_cache_keys.find(combined_key);
    if (q == m_psos_cache_keys.end())
    {
        auto psos_cache_keys_insertion_position =
            m_psos_cache_keys.insert(std::make_pair(
                combined_key,
                std::make_pair(graphics_pso_cache_storage::iterator{}, compute_pso_cache_storage::iterator{}))).first;


        m_compute_pso_compilation_tasks.emplace_back(
            psos_cache_keys_insertion_position->first, globals,
            versioned_descriptor.descriptor(), versioned_descriptor.timestamp()
        );

        compute_pso_cache_storage::iterator p = --m_compute_pso_compilation_tasks.end();
        psos_cache_keys_insertion_position->second.second = p;

        tasks::ComputePSOCompilationTask& new_compute_pso_compilation_task_ref = *p;
        new_pso_compilation_task = &new_compute_pso_compilation_task_ref;
    }
    else
    {
        return &(*q->second.second);
    }

    return new_pso_compilation_task;
}

PSOCompilationTaskCache::graphics_pso_cache_storage& PSOCompilationTaskCache::graphicsPSOStorage()
{
    return m_graphics_pso_compilation_tasks;
}

PSOCompilationTaskCache::graphics_pso_cache_storage const& PSOCompilationTaskCache::graphicsPSOStorage() const
{
    return m_graphics_pso_compilation_tasks;
}

PSOCompilationTaskCache::compute_pso_cache_storage& PSOCompilationTaskCache::computePSOStorage()
{
    return m_compute_pso_compilation_tasks;
}

PSOCompilationTaskCache::compute_pso_cache_storage const& PSOCompilationTaskCache::computePSOStorage() const
{
    return m_compute_pso_compilation_tasks;
}