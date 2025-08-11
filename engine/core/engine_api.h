#ifndef LEXGINE_CORE_ENGINE_API_H
#define LEXGINE_CORE_ENGINE_API_H

#include "engine/preprocessing/preprocessor_tokens.h"
#include "engine/core/misc/misc.h"

namespace lexgine::core{

//! Version of the renderer
enum class LEXGINE_CPP_API EngineApi
{
    Direct3D12,
    Vulkan,
    Metal,
    OpenGL46    // not sure if ever gets implemented
};

enum class LEXGINE_CPP_API MSAAMode {
    none = 1,
    msaa2x = 2,
    msaa4x = 4,
    msaa8x = 8,
    msaa16x = 16
};

class LEXGINE_CPP_API DEPENDS_ON(EngineApi) EngineApiAwareObject
{
public:
    EngineApiAwareObject(EngineApi m_engine_api)
        : m_engine_api{ m_engine_api } {}

    //! Returns graphics API implementing the back end of this object
    LEXGINE_CPP_API EngineApi engineApi() const { return m_engine_api; }

private:
    EngineApi m_engine_api;
};
    
}

#endif