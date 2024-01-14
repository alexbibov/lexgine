#ifndef LEXGINE_CORE_GLOBALS_H
#define LEXGINE_CORE_GLOBALS_H

#include <map>
#include <cassert>

#include <engine/core/misc/hashed_string.h>
#include <engine/core/entity.h>
#include <engine/core/class_names.h>
#include <engine/core/engine_api.h>
#include <engine/core/lexgine_core_fwd.h>
#include <engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h>
#include <engine/core/dx/d3d12/task_caches/lexgine_core_dx_d3d12_task_caches_fwd.h>


namespace lexgine::core {

class Globals : NamedEntity<class_names::Globals>
{
private:

    using global_object_pool_type = std::map<misc::HashedString, void*>;


private:

    global_object_pool_type m_global_object_pool;


private:

    void* find(misc::HashedString const& hashed_name);
    void const* find(misc::HashedString const& hashed_name) const;

    void put(misc::HashedString const& hashed_name, void* p_object);


public:

    template<typename T,
        typename = typename std::enable_if<
        std::is_trivially_constructible<T>::value
        || std::is_default_constructible<T>::value>::type>
    T* get()
    {
        misc::HashedString hashed_type_name{ typeid(T).name() };
        void* rv{ nullptr };

        if ((rv = find(hashed_type_name)))
            return static_cast<T*>(rv);

        rv = new T{};
        put(hashed_type_name, rv);
        return static_cast<T*>(rv);
    }

    template<typename T,
        typename = typename std::enable_if<
        !std::is_trivially_constructible<T>::value
        && !std::is_default_constructible<T>::value>::type,
        typename = void>
    T* get()
    {
        return static_cast<T*>(find(misc::HashedString{ typeid(T).name() }));
    }

    template<typename T>
    T const* get() const
    {
        misc::HashedString hashed_type_name{ typeid(T).name() };
        void const* rv = find(hashed_type_name);
        assert(rv);

        return static_cast<T const*>(rv);
    }

    template<typename T>
    void put(T* obj)
    {
        static_assert(!std::is_const<T>::value, "constant objects cannot be put into object pool \"Globals\"");
        misc::HashedString hashed_type_name{ typeid(T).name() };
        put(hashed_type_name, obj);
    }
};



//! Construct the main and the most generalist part of the Globals object pool
class MainGlobalsBuilder
{
public:
    void defineEngineApi(EngineApi api);
    void defineGlobalSettings(GlobalSettings& global_settings);
    void defineLoggingStreams(LoggingStreams& logging_streams);
    void registerDxResourceFactory(dx::d3d12::DxResourceFactory& dx_resource_factory);
    void registerHLSLCompilationTaskCache(dx::d3d12::task_caches::HLSLCompilationTaskCache& shader_cache);
    void registerPSOCompilationTaskCache(dx::d3d12::task_caches::PSOCompilationTaskCache& pso_cache);
    void registerRootSignatureCompilationTaskCache(dx::d3d12::task_caches::RootSignatureCompilationTaskCache& rs_cache);


    std::unique_ptr<Globals> build();

private:
    EngineApi m_engine_api;
    GlobalSettings* m_global_settings;
    dx::d3d12::DxResourceFactory* m_dx_resource_factory;
    dx::d3d12::task_caches::HLSLCompilationTaskCache* m_shader_cache;
    dx::d3d12::task_caches::PSOCompilationTaskCache* m_pso_cache;
    dx::d3d12::task_caches::RootSignatureCompilationTaskCache* m_rs_cache;
    LoggingStreams* m_logging_streams;
};


}

#endif