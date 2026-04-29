#ifndef LEXGINE_CORE_FWD_H
#define LEXGINE_CORE_FWD_H

#include <concepts>
#include <cstddef>
#include <string>

namespace lexgine::core {

class AbstractVertexAttributeSpecification;
class BlendDescriptor;
class DataBlob;
class DataChunk;
class D3DDataBlob;
class DepthStencilDescriptor;
class Entity;
class ErrorBehavioral;
class Exception;
class FilterPack;
class GlobalSettings;

class Globals;
class ProvidesGlobals
{
public:
    ProvidesGlobals(Globals& globals) : m_globals{ globals } {}
    Globals& globals() { return m_globals; }
    Globals const& globals() const { return m_globals; }

protected:
    Globals& m_globals;
};

class RasterizerDescriptor;
class ShaderSourceCodePreprocessor;
class Viewport;
template<typename T>
concept StreamedCacheCompatibleKey = requires(T v1, T v2)
{
    { T::serialized_size } -> std::convertible_to<std::size_t>;
    { v1.toString() } -> std::convertible_to<std::string>;
    { v1 < v2 } -> std::convertible_to<bool>;
    { v1 == v2 } -> std::convertible_to<bool>;
    requires requires {
        static_cast<void (T::*)(void*) const>(&T::serialize);
        static_cast<void (T::*)(void const*)>(&T::deserialize);
    };
};

template<StreamedCacheCompatibleKey Key, std::size_t cluster_size> class StreamedCache;
template<StreamedCacheCompatibleKey Key, std::size_t cluster_size> class StreamedCacheConcurrencySentinel;
class ProfilingService;
class CPUTaskProfilingService;
class GPUTaskProfilingService;
struct RenderingConfiguration;

}

#endif
