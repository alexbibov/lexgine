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

UUID EntityID::asUUID() const
{
    size_t const part1 = owningThread();
    uint64_t const part2 = m_id_counter;

    return UUID{
        part1 & 0xFFFFFFFF,
        (part1 >> 32) & 0xFFFF,
        (part1 >> 48) & 0xFFFF,
        {part2 & 0xFF, (part2 >> 8) & 0xFF, (part2 >> 16) & 0xFF, (part2 >> 24) & 0xFF,
        (part2 >> 32) & 0xFF, (part2 >> 40) & 0xFF, (part2 >> 48) & 0xFF, (part2 >> 56) & 0xFF}
    };
}




std::atomic<int64_t> Entity::m_alive_entities{ 0 };

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

int64_t lexgine::core::Entity::aliveEntities()
{
    return m_alive_entities.load(std::memory_order_acquire);
}

Entity::Entity() :
    m_string_name{ "unnamed(id=" + m_id.toString() + ")" }
{
    m_alive_entities.fetch_add(1LL, std::memory_order_acq_rel);
}

Entity::Entity(Entity const& other) :
    m_id{},
    m_string_name{ other.m_string_name }
{
    m_alive_entities.fetch_add(1LL, std::memory_order_acq_rel);
}

Entity::Entity(Entity&& other) :
    m_id{ std::move(other.m_id) },
    m_string_name{ std::move(other.m_string_name) }
{
    m_alive_entities.fetch_add(1LL, std::memory_order_acq_rel);
}

Entity& Entity::operator=(Entity const& other)
{
    if (this == &other)
        return *this;

    m_string_name = other.m_string_name;
    return *this;
}

Entity& lexgine::core::Entity::operator=(Entity&& other)
{
    if (this == &other)
        return *this;

    m_string_name = std::move(other.m_string_name);
    return *this;
}

Entity::~Entity()
{
    m_alive_entities.fetch_add(-1LL, std::memory_order_acq_rel);
}