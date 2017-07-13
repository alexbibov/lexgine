#ifndef LEXGINE_CORE_GLOBALS_H

#include <map>
#include <string>
#include <memory>

#include "../../../3rd_party/SpookyHash/SpookyV2.h"


namespace lexgine { namespace core {

class Globals
{
private:

    class StringAndHash
    {
    private:
        std::string m_string;
        uint64_t m_hash;

    public:
        StringAndHash(spooky_hash_v2::SpookyHash& hash_generator, uint64_t seed, std::string const& str);

        bool operator<=(StringAndHash const& other) const;
        bool operator==(StringAndHash const& other) const;
    };

    class impl;

    
    std::unique_ptr<impl> m_impl;

    static Globals* m_p_self;
    std::map<StringAndHash, void*> m_global_object_pool;


    Globals();

    Globals(Globals const&) = delete;
    Globals(Globals&&) = delete;

    Globals& operator=(Globals const&) = delete;
    Globals& operator=(Globals&&) = delete;


    void* get(char const* object_type_name, void* p_object);
    void const* get(char const* object_type_name) const;


public:

    ~Globals();

    static Globals* create();
    static void destroy();


    template<typename T>
    T* get()
    {
        return static_cast<T*>(get(typeid(T).name()));
    }

    template<typename T>
    T const* get() const
    {
        return static_cast<T const*>(get(typeid(T).name()));
    }
};

}}

#define LEXGINE_CORE_GLOBALS_H
#endif