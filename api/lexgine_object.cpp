#include <cassert>

#include "lexgine_object.h"
#include <engine/core/entity.h>

using namespace lexgine::api;
using namespace lexgine::core;

namespace{
    
void (LEXGINE_CALL *api__lexgineCoreObjectGetUUID)(void const*, UUID&) = nullptr;
void (LEXGINE_CALL *api__lexgineCoreObjectGetStringName)(void const*, std::string&) = nullptr;
void (LEXGINE_CALL *api__lexgineCoreObjectSetStringName)(void*, std::string const&) = nullptr;
uint32_t (LEXGINE_CALL *api__lexgineCoreGetAliveEntitiesCount)() = nullptr;
size_t (LEXGINE_CALL *api__lexgineIocTraitsGetImportedOpaqueClassSize)(lexgine::common::ImportedOpaqueClass) = nullptr;
    
}

LinkResult LexgineObject::link(HMODULE module)
{
    LinkResult rv{module};
    api__lexgineCoreObjectGetUUID = reinterpret_cast<decltype(api__lexgineCoreObjectGetUUID)>(rv.attemptLink("lexgineCoreObjectGetUUID"));
    api__lexgineCoreObjectGetStringName = reinterpret_cast<decltype(api__lexgineCoreObjectGetStringName)>(rv.attemptLink("lexgineCoreObjectGetStringName"));
    api__lexgineCoreObjectSetStringName = reinterpret_cast<decltype(api__lexgineCoreObjectSetStringName)>(rv.attemptLink("lexgineCoreObjectSetStringName"));
    api__lexgineCoreGetAliveEntitiesCount = reinterpret_cast<decltype(api__lexgineCoreGetAliveEntitiesCount)>(rv.attemptLink("lexgineCoreGetAliveEntitiesCount"));
    api__lexgineIocTraitsGetImportedOpaqueClassSize = reinterpret_cast<decltype(api__lexgineIocTraitsGetImportedOpaqueClassSize)>(rv.attemptLink("lexgineIocTraitsGetImportedOpaqueClassSize"));
    return rv;
}

LexgineObject::LexgineObject(lexgine::common::ImportedOpaqueClass ioc_name)
{
    assert(api__lexgineIocTraitsGetImportedOpaqueClassSize);
    size_t ioc_size = api__lexgineIocTraitsGetImportedOpaqueClassSize(ioc_name);
    m_impl_buf = std::make_unique<uint8_t[]>(ioc_size);
}


void* LexgineObject::getNative() const { return m_impl_buf.get(); }


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


