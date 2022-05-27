#ifndef LEXGINE_API_LEXGINE_OBJECT_H
#define LEXGINE_API_LEXGINE_OBJECT_H

#include <cstdint>
#include <string>
#include <memory>
#include <guiddef.h>
#include <api/link_result.h>
#include <api/preprocessing/preprocessor_tokens.h>
#include <common/ioc_traits.h>

namespace lexgine::api {

class LexgineObject
{
public:    // runtime linking infrastructure
    static LinkResult link(HMODULE module);    //! Runtime link interface
    
public:
    GUID asUUID() const;    //! returns UUID of the object
    std::string getStringName() const;    //! returns user-friendly string name of the object
    void setStringName(std::string const& new_name);    //! assigns new string name to the object
    static uint64_t aliveEntities();    //! returns total population of instantiated engine objects. Useful for debugging purposes.
        
protected:
    LexgineObject(lexgine::common::ImportedOpaqueClass ioc_name);
    void* getNative() const;
    
private:
    std::unique_ptr<uint8_t[]> m_impl_buf;
};

}

#endif