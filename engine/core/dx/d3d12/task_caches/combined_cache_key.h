#ifndef LEXGINE_CORE_DX_D3D12_TASK_CACHES_COMBINED_CACHE_KEY_H
#define LEXGINE_CORE_DX_D3D12_TASK_CACHES_COMBINED_CACHE_KEY_H

#include "hlsl_compilation_task_cache.h"
#include "pso_compilation_task_cache.h"
#include "root_signature_compilation_task_cache.h"


namespace lexgine::core::dx::d3d12::task_caches {


class CombinedCacheKey
{
private:
    enum class cache_entry_type : unsigned char
    {
        shader, pipeline_state_object, root_signature
    };

public:
    static constexpr size_t serialized_size = sizeof(cache_entry_type) +
        (std::max)(RootSignatureCompilationTaskCache::Key::serialized_size,
        (std::max)(HLSLCompilationTaskCache::Key::serialized_size,
            PSOCompilationTaskCache::Key::serialized_size));

    CombinedCacheKey(RootSignatureCompilationTaskCache::Key const& key);
    CombinedCacheKey(PSOCompilationTaskCache::Key const& key);
    CombinedCacheKey(HLSLCompilationTaskCache::Key const& key);

    CombinedCacheKey();
    ~CombinedCacheKey();
    CombinedCacheKey(CombinedCacheKey const& other);
    CombinedCacheKey(CombinedCacheKey&& other);

    CombinedCacheKey& operator=(CombinedCacheKey const& other);
    CombinedCacheKey& operator=(CombinedCacheKey&& other);


    std::string toString() const;

    void serialize(void* p_serialization_blob) const;
    void deserialize(void const* p_serialization_blob);

    bool operator<(CombinedCacheKey const& other) const;
    bool operator==(CombinedCacheKey const& other) const;

    operator RootSignatureCompilationTaskCache::Key const&() const;
    operator PSOCompilationTaskCache::Key const&() const;
    operator HLSLCompilationTaskCache::Key const&() const;

private:
    cache_entry_type m_entry_type;

    union maintained_key{
        RootSignatureCompilationTaskCache::Key rs;
        PSOCompilationTaskCache::Key pso;
        HLSLCompilationTaskCache::Key hlsl;

        maintained_key(RootSignatureCompilationTaskCache::Key const& key);
        maintained_key(PSOCompilationTaskCache::Key const& key);
        maintained_key(HLSLCompilationTaskCache::Key const& key);

        maintained_key(cache_entry_type type, maintained_key const& other);
        maintained_key(cache_entry_type type, maintained_key&& other);

    } m_key;
};

}

#endif
