#ifndef LEXGINE_CORE_GLOBALS_H

#include <map>
#include <string>
#include <memory>

#include "../../3rd_party/SpookyHash/SpookyV2.h"
#include "misc/hashed_string.h"


namespace lexgine { namespace core {

class Globals
{
private:
    using global_object_pool_type = std::map<misc::HashedString, void*>;


private:

    static Globals* m_p_self;
    global_object_pool_type m_global_object_pool;

    spooky_hash_v2::SpookyHash m_spooky_hash_generator;
    unsigned long long m_hash_seed;


private:

    Globals();

    Globals(Globals const&) = delete;
    Globals(Globals&&) = delete;

    Globals& operator=(Globals const&) = delete;
    Globals& operator=(Globals&&) = delete;


    void* find(misc::HashedString const& hashed_name);
    void const* find(misc::HashedString const& hashed_name) const;

    bool put(misc::HashedString const& hashed_name, void* p_object);

    misc::HashedString attachHasToString(std::string const& str);


public:

    ~Globals();

    static Globals* create();
    static void destroy();


    template<typename T>
    T* get()
    {
        misc::HashedString hashed_type_name{ attachHasToString(typeid(T).name()) };
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
        misc::HashedString hashed_type_name{ attachHasToString(typeid(T).name()) };
        void const* rv = find(hashed_type_name);
        assert(rv);

        return static_cast<T const*>(rv);
    }

    template<typename T>
    bool put(T* obj)
    {
        misc::HashedString hashed_type_name{ attachHasToString(typeid(T).name()) };
        return put(hashed_type_name, obj);
    }
};

}}

#define LEXGINE_CORE_GLOBALS_H
#endif