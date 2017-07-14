#ifndef LEXGINE_CORE_GLOBALS_H

#include <map>
#include <string>
#include <memory>
#include <vector>
#include <ostream>

#include "misc/hashed_string.h"
#include "misc/optional.h"
#include "global_settings.h"


namespace lexgine { namespace core {

class Globals
{
private:
    
    using global_object_pool_type = std::map<misc::HashedString, void*>;


private:

    global_object_pool_type m_global_object_pool;


private:

    void* find(misc::HashedString const& hashed_name);
    void const* find(misc::HashedString const& hashed_name) const;

    bool put(misc::HashedString const& hashed_name, void* p_object);


public:

    template<typename T>
    T* get()
    {
        misc::HashedString hashed_type_name{ typeid(T).name() };
        void* rv{ nullptr };

        if ((rv = find(hashed_type_name)))
            return static_cast<T*>(rv);
        
        rv = new T{};
        put(hashed_type_name, rv);
        return rv;
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
    bool put(T* obj)
    {
        misc::HashedString hashed_type_name{ typeid(T).name() };
        return put(hashed_type_name, obj);
    }

    template<typename T>
    bool put(T const*)
    {
        static_assert(false, "constant objects cannot be put into object pool \"Globals\"");
    }
};



//! Construct the main and the most generalist part of the Globals object pool
class MainGlobalsBuilder
{
private:
    misc::Optional<GlobalSettings> m_global_settings;
    std::vector<std::ostream*> m_thread_logs;

public:
    void defineGlobalSettings(GlobalSettings const& global_settings);
    void registerThreadLog(uint8_t worker_id, std::ostream* logging_output_stream);

    Globals build();
};


}}

#define LEXGINE_CORE_GLOBALS_H
#endif