#ifndef LEXGINE_CORE_FWD_H
#define LEXGINE_CORE_FWD_H

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
template<typename Key, size_t cluster_size> class StreamedCache;
class ProfilingService;
class CPUTaskProfilingService;
class GPUTaskProfilingService;
struct RenderingConfiguration;

}

#endif
