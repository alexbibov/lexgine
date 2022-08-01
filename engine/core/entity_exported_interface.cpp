#include <guiddef.h>
#include <common/ioc_traits.h>
#include "engine/core/entity.h"
#include "engine/preprocessing/preprocessor_tokens.h"

namespace lexgine::core {
	
extern "C"{

LEXGINE_API void LEXGINE_CALL lexgineCoreObjectGetUUID(void const* p_instance, GUID& uuid)
{
	uuid = static_cast<Entity const*>(p_instance)->getId().asUUID();
}

LEXGINE_API void LEXGINE_CALL lexgineCoreObjectGetStringName(void const* p_instance, std::string& object_name)
{
	object_name = static_cast<Entity const*>(p_instance)->getStringName();
}

LEXGINE_API void LEXGINE_CALL lexgineCoreObjectSetStringName(void* p_instance, std::string const& new_object_name)
{
	static_cast<Entity*>(p_instance)->setStringName(new_object_name);
}

LEXGINE_API uint64_t LEXGINE_CALL lexgineCoreGetAliveEntitiesCount()
{
	return Entity::aliveEntities();
}

}

}