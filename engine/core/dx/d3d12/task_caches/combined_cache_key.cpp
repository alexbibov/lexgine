#include "combined_cache_key.h"
#include "engine/core/misc/strict_weak_ordering.h"

#include <cassert>

using namespace lexgine::core::dx::d3d12::task_caches;

CombinedCacheKey::CombinedCacheKey(RootSignatureCompilationTaskCache::Key const& key):
    m_key{ key },
    m_entry_type{ cache_entry_type::root_signature }
{
}

CombinedCacheKey::CombinedCacheKey(PSOCompilationTaskCache::Key const& key) :
    m_key{ key },
    m_entry_type{ cache_entry_type::pipeline_state_object }
{
}

CombinedCacheKey::CombinedCacheKey(HLSLCompilationTaskCache::Key const& key) :
    m_key{ key },
    m_entry_type{ cache_entry_type::shader }
{
}

CombinedCacheKey::CombinedCacheKey():
    CombinedCacheKey{ RootSignatureCompilationTaskCache::Key{} }
{
}

CombinedCacheKey::~CombinedCacheKey()
{
    switch (m_entry_type)
    {
    case CombinedCacheKey::cache_entry_type::shader:
        m_key.hlsl.~Key();
        break;

    case CombinedCacheKey::cache_entry_type::pipeline_state_object:
        m_key.pso.~Key();
        break;

    case CombinedCacheKey::cache_entry_type::root_signature:
        m_key.rs.~Key();
        break;
    }
}

CombinedCacheKey::CombinedCacheKey(CombinedCacheKey const& other):
    m_entry_type{ other.m_entry_type },
    m_key{ m_entry_type, other.m_key }
{
    
}

CombinedCacheKey::CombinedCacheKey(CombinedCacheKey&& other) :
    m_entry_type{ other.m_entry_type },
    m_key{ m_entry_type, std::move(other.m_key) }
{

}

CombinedCacheKey& CombinedCacheKey::operator=(CombinedCacheKey const& other)
{
    if (this == &other)
        return *this;

    if (m_entry_type == other.m_entry_type)
    {
        switch (m_entry_type)
        {
        case CombinedCacheKey::cache_entry_type::shader:
            m_key.hlsl = other.m_key.hlsl;
            break;

        case CombinedCacheKey::cache_entry_type::pipeline_state_object:
            m_key.pso = other.m_key.pso;
            break;

        case CombinedCacheKey::cache_entry_type::root_signature:
            m_key.rs = other.m_key.rs;
            break;
        }
    }
    else
    {
        switch (m_entry_type)
        {
        case CombinedCacheKey::cache_entry_type::shader:
            m_key.hlsl.~Key();
            break;

        case CombinedCacheKey::cache_entry_type::pipeline_state_object:
            m_key.pso.~Key();
            break;

        case CombinedCacheKey::cache_entry_type::root_signature:
            m_key.rs.~Key();
            break;
        }

        switch (other.m_entry_type)
        {
        case CombinedCacheKey::cache_entry_type::shader:
            new(&m_key.hlsl) HLSLCompilationTaskCache::Key{ other.m_key.hlsl };
            break;

        case CombinedCacheKey::cache_entry_type::pipeline_state_object:
            new(&m_key.pso) PSOCompilationTaskCache::Key{ other.m_key.pso };
            break;

        case CombinedCacheKey::cache_entry_type::root_signature:
            new(&m_key.rs) RootSignatureCompilationTaskCache::Key{ other.m_key.rs };
            break;
        }

        m_entry_type = other.m_entry_type;
    }

    return *this;
}

CombinedCacheKey& CombinedCacheKey::operator=(CombinedCacheKey&& other)
{
    if (this == &other)
        return *this;

    if (m_entry_type == other.m_entry_type)
    {
        switch (m_entry_type)
        {
        case CombinedCacheKey::cache_entry_type::shader:
            m_key.hlsl = std::move(other.m_key.hlsl);
            break;

        case CombinedCacheKey::cache_entry_type::pipeline_state_object:
            m_key.pso = std::move(other.m_key.pso);
            break;

        case CombinedCacheKey::cache_entry_type::root_signature:
            m_key.rs = std::move(other.m_key.rs);
            break;
        }
    }
    else
    {
        switch (m_entry_type)
        {
        case CombinedCacheKey::cache_entry_type::shader:
            m_key.hlsl.~Key();
            break;

        case CombinedCacheKey::cache_entry_type::pipeline_state_object:
            m_key.pso.~Key();
            break;

        case CombinedCacheKey::cache_entry_type::root_signature:
            m_key.rs.~Key();
            break;
        }

        switch (other.m_entry_type)
        {
        case CombinedCacheKey::cache_entry_type::shader:
            new(&m_key.hlsl) HLSLCompilationTaskCache::Key{ std::move(other.m_key.hlsl) };
            break;

        case CombinedCacheKey::cache_entry_type::pipeline_state_object:
            new(&m_key.pso) PSOCompilationTaskCache::Key{ std::move(other.m_key.pso) };
            break;

        case CombinedCacheKey::cache_entry_type::root_signature:
            new(&m_key.rs) RootSignatureCompilationTaskCache::Key{ std::move(other.m_key.rs) };
            break;
        }

        m_entry_type = other.m_entry_type;
    }

    return *this;
}

std::string CombinedCacheKey::toString() const
{
    switch (m_entry_type)
    {
    case CombinedCacheKey::cache_entry_type::shader:
        return m_key.hlsl.toString();
    case CombinedCacheKey::cache_entry_type::pipeline_state_object:
        return m_key.pso.toString();
    case CombinedCacheKey::cache_entry_type::root_signature:
        return m_key.rs.toString();
    default:
        return "unknown combined cache key type";
    }
}

void CombinedCacheKey::serialize(void* p_serialization_blob) const
{
    uint8_t* ptr = static_cast<uint8_t*>(p_serialization_blob);
    memcpy(ptr, &m_entry_type, sizeof(m_entry_type)); ptr += sizeof(cache_entry_type);

    switch (m_entry_type)
    {
    case CombinedCacheKey::cache_entry_type::shader:
        m_key.hlsl.serialize(reinterpret_cast<void*>(ptr));
        break;

    case CombinedCacheKey::cache_entry_type::pipeline_state_object:
        m_key.pso.serialize(reinterpret_cast<void*>(ptr));
        break;

    case CombinedCacheKey::cache_entry_type::root_signature:
        m_key.rs.serialize(reinterpret_cast<void*>(ptr));
        break;
    }
}

void CombinedCacheKey::deserialize(void const* p_serialization_blob)
{
    uint8_t const* ptr = static_cast<uint8_t const*>(p_serialization_blob);
    memcpy(&m_entry_type, ptr, sizeof(m_entry_type)); ptr += sizeof(m_entry_type);

    switch (m_entry_type)
    {
    case CombinedCacheKey::cache_entry_type::shader:
        m_key.hlsl.deserialize(reinterpret_cast<void const*>(ptr));
        break;

    case CombinedCacheKey::cache_entry_type::pipeline_state_object:
        m_key.pso.deserialize(reinterpret_cast<void const*>(ptr));
        break;

    case CombinedCacheKey::cache_entry_type::root_signature:
        m_key.rs.deserialize(reinterpret_cast<void const*>(ptr));
        break;
    }
}

bool CombinedCacheKey::operator<(CombinedCacheKey const& other) const
{
    SWO_STEP(static_cast<unsigned char>(m_entry_type), < , static_cast<unsigned char>(other.m_entry_type));

    switch (m_entry_type)
    {
    case CombinedCacheKey::cache_entry_type::shader:
        SWO_END(m_key.hlsl, < , other.m_key.hlsl);
        break;

    case CombinedCacheKey::cache_entry_type::pipeline_state_object:
        SWO_END(m_key.pso, <, other.m_key.pso);
        break;

    case CombinedCacheKey::cache_entry_type::root_signature:
        SWO_END(m_key.rs, <, other.m_key.rs);
        break;
    }

    return false;
}

bool CombinedCacheKey::operator==(CombinedCacheKey const& other) const
{
    if (m_entry_type != other.m_entry_type) return false;

    switch (m_entry_type)
    {
    case CombinedCacheKey::cache_entry_type::shader:
        return m_key.hlsl == other.m_key.hlsl;

    case CombinedCacheKey::cache_entry_type::pipeline_state_object:
        return m_key.pso == other.m_key.pso;

    case CombinedCacheKey::cache_entry_type::root_signature:
        return m_key.rs == other.m_key.rs;
    }

    return false;
}

CombinedCacheKey::operator RootSignatureCompilationTaskCache::Key const&() const
{
    assert(m_entry_type == cache_entry_type::root_signature);
    return m_key.rs;
}

CombinedCacheKey::operator PSOCompilationTaskCache::Key const&() const
{
    assert(m_entry_type == cache_entry_type::pipeline_state_object);
    return m_key.pso;
}

CombinedCacheKey::operator HLSLCompilationTaskCache::Key const&() const
{
    assert(m_entry_type == cache_entry_type::shader);
    return m_key.hlsl;
}


CombinedCacheKey::maintained_key::maintained_key(RootSignatureCompilationTaskCache::Key const& key)
{
    new(&rs) RootSignatureCompilationTaskCache::Key{ key };
}

CombinedCacheKey::maintained_key::maintained_key(PSOCompilationTaskCache::Key const& key)
{
    new(&pso) PSOCompilationTaskCache::Key{ key };
}

CombinedCacheKey::maintained_key::maintained_key(HLSLCompilationTaskCache::Key const& key)
{
    new(&hlsl) HLSLCompilationTaskCache::Key{ key };
}

CombinedCacheKey::maintained_key::maintained_key(cache_entry_type type, maintained_key const& other)
{
    switch (type)
    {
    case CombinedCacheKey::cache_entry_type::shader:
        new(&hlsl) HLSLCompilationTaskCache::Key{ other.hlsl };
        break;

    case CombinedCacheKey::cache_entry_type::pipeline_state_object:
        new(&pso) PSOCompilationTaskCache::Key{ other.pso };
        break;

    case CombinedCacheKey::cache_entry_type::root_signature:
        new(&rs) RootSignatureCompilationTaskCache::Key{ other.rs };
        break;
    }
}

CombinedCacheKey::maintained_key::maintained_key(cache_entry_type type, maintained_key&& other)
{
    switch (type)
    {
    case CombinedCacheKey::cache_entry_type::shader:
        new(&hlsl) HLSLCompilationTaskCache::Key{ std::move(other.hlsl) };
        break;

    case CombinedCacheKey::cache_entry_type::pipeline_state_object:
        new(&pso) PSOCompilationTaskCache::Key{ std::move(other.pso) };
        break;

    case CombinedCacheKey::cache_entry_type::root_signature:
        new(&rs) RootSignatureCompilationTaskCache::Key{ std::move(other.rs) };
        break;
    }
}
