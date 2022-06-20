#include <cassert>
#include <api/preprocessing/preprocessor_tokens.h>
#include "lexgine_object.h"

using namespace lexgine::api;

namespace{
    
void (LEXGINE_CALL *api__lexgineCoreObjectGetUUID)(void const*, GUID&) = nullptr;
void (LEXGINE_CALL *api__lexgineCoreObjectGetStringName)(void const*, std::string&) = nullptr;
void (LEXGINE_CALL *api__lexgineCoreObjectSetStringName)(void*, std::string const&) = nullptr;
uint64_t (LEXGINE_CALL *api__lexgineCoreGetAliveEntitiesCount)() = nullptr;
    
}

LinkResult LexgineObject::link(HMODULE module)
{
    LinkResult rv{module};
    api__lexgineCoreObjectGetUUID = reinterpret_cast<decltype(api__lexgineCoreObjectGetUUID)>(rv.attemptLink("lexgineCoreObjectGetUUID"));
    api__lexgineCoreObjectGetStringName = reinterpret_cast<decltype(api__lexgineCoreObjectGetStringName)>(rv.attemptLink("lexgineCoreObjectGetStringName"));
    api__lexgineCoreObjectSetStringName = reinterpret_cast<decltype(api__lexgineCoreObjectSetStringName)>(rv.attemptLink("lexgineCoreObjectSetStringName"));
    api__lexgineCoreGetAliveEntitiesCount = reinterpret_cast<decltype(api__lexgineCoreGetAliveEntitiesCount)>(rv.attemptLink("lexgineCoreGetAliveEntitiesCount"));
    return rv;
}

LexgineObject::LexgineObject(lexgine::common::ImportedOpaqueClass ioc_name)
	: Ioc{ioc_name}
{
}

GUID LexgineObject::asUUID() const
{
    assert(api__lexgineCoreObjectGetUUID);
    GUID rv{};
    api__lexgineCoreObjectGetUUID(getNative(), rv);
    return rv;
}

std::string LexgineObject::getStringName() const
{
    assert(api__lexgineCoreObjectGetStringName);
    std::string rv{};
    api__lexgineCoreObjectGetStringName(getNative(), rv);
    return rv;
}

void LexgineObject::setStringName(std::string const& new_name)
{
    assert(api__lexgineCoreObjectSetStringName);
    api__lexgineCoreObjectSetStringName(getNative(), new_name);
}

uint64_t LexgineObject::aliveEntities()
{
    assert(api__lexgineCoreGetAliveEntitiesCount);
    return api__lexgineCoreGetAliveEntitiesCount();
}


