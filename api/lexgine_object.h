#ifndef LEXGINE_API_LEXGINE_OBJECT_H
#define LEXGINE_API_LEXGINE_OBJECT_H

#include <cinttypes>
#include <string>
#include <engine/runtime/preprocessor_tokens.h>
#include <guiddef.h>

namespace lexgine::api {

class LEXGINE_API LexgineObject
{
public:
    GUID asUUID() const;    //! returns UUID of the object
    std::string getStringName() const;    //! returns user-friendly string name of the object
    void setStringName(std::string const& new_name);    //! assigns new string name to the object
    static uint64_t aliveEntities();    //! returns total population of instantiated engine objects. Useful for debugging purposes.
		
protected:
    LexgineObject(void* ptr);
	void* getNative() const;

private:
    void* m_ptr;
};

}

#endif