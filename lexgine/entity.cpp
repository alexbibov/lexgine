#include "entity.h"

using namespace lexgine::core;

thread_local uint64_t EntityID::m_id_counter = 0;


EntityID::EntityID() :
    m_owning_thread{ std::this_thread::get_id() },
    m_id{ ++m_id_counter }
{

}

bool EntityID::operator==(EntityID const& other) const
{
    return m_owning_thread == other.m_owning_thread
        && m_id == other.m_id;
}

bool EntityID::operator<(EntityID const& other) const
{
    return m_owning_thread < other.m_owning_thread
        || (m_owning_thread == other.m_owning_thread && m_id < other.m_id);
}

bool EntityID::operator<=(EntityID const& other) const
{
    return *this < other || *this == other;
}

bool EntityID::operator>(EntityID const& other) const
{
    return m_owning_thread > other.m_owning_thread
        || (m_owning_thread == other.m_owning_thread && m_id > other.m_id);
}

bool EntityID::operator>=(EntityID const& other) const
{
    return *this > other || *this == other;
}

bool EntityID::operator!=(EntityID const& other) const
{
    return !(*this == other);
}

size_t EntityID::owningThread() const
{
    std::hash<std::thread::id> hasher{};
    return hasher(m_owning_thread);
}

std::string EntityID::toString() const
{
    return std::to_string(owningThread()) + "_" + std::to_string(m_id);
}




thread_local uint64_t Entity::m_alive_entities = 0;

EntityID Entity::getId() const
{
    return m_id;
}

std::string Entity::getStringName() const
{
    return m_string_name;
}

void Entity::setStringName(std::string const& entity_string_name)
{
    m_string_name = entity_string_name;
}

uint64_t lexgine::core::Entity::aliveEntities()
{
    return m_alive_entities;
}

Entity::Entity() :
    m_string_name{ "unnamed(id=" + m_id.toString() + ")" }
{
    ++m_alive_entities;
}

Entity::Entity(Entity const& other):
    m_id{},
    m_string_name{ other.m_string_name }
{
    ++m_alive_entities;
}

Entity::Entity(Entity&& other) : 
    m_id{ std::move(other.m_id) },
    m_string_name{ std::move(other.m_string_name) }
{
    ++m_alive_entities;
}

Entity& Entity::operator=(Entity const & other)
{
    if (this == &other)
        return *this;
    
    m_string_name = other.m_string_name;
    return *this;
}

Entity & lexgine::core::Entity::operator=(Entity&& other)
{
    if (this == &other)
        return *this;

    m_string_name = std::move(other.m_string_name);
    return *this;
}

Entity::~Entity()
{
    --m_alive_entities;
}