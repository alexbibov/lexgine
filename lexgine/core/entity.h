#ifndef LEXGINE_CORE_ENTITY_H

#include <thread>
#include <string>

#include "error_behavioral.h"

namespace lexgine { namespace core {


//! Implements identifier of the engine objects, which is guaranteed to remain unique for each object
//! created by the engine during the life time of the application. This object is OS-agnostic.
class EntityID
{
public:
    EntityID();    //! initializes new identifier based on the calling thread and on the number of entities previously created by this thread

    bool operator==(EntityID const& other) const;
    bool operator<(EntityID const& other) const;
    bool operator<=(EntityID const& other) const;
    bool operator>(EntityID const& other) const;
    bool operator>=(EntityID const& other) const;
    bool operator!=(EntityID const& other) const;

    //! Returns numeric hash identifier of the thread that has created the object referred by the id
    size_t owningThread() const;

    //! Returns string representation of the identifier
    std::string toString() const;

private:
    static thread_local uint64_t m_id_counter;    //!< identifier counter local to each thread

    std::thread::id m_owning_thread;    //!< identifier of the thread that was used to create the object having this id
    uint64_t m_id;    //!< numeric identifier of the object
};


//! Almost every class from any nested namespace from lexgine::core (except the types located in lexgine::core::misc and lexgine::core::math and also few other minor types) is derived from this class, i.e.
//! every "significant enough" notion in the engine is an "entity". Entity provides very basic functionality such as object unique identification and creation-destruction logging.
//! Note also that the types exlicitly residing in lexgine::core (i.e. lexgine::core::BlendDescriptor) do never derive from this type.
//! Naturally, this object is OS-agnostic
//! Note that classes with name Window located in lexgine::osinteraction also for now derive from this type (this is subject to change in the future as these classes are tailored to
//! each supported operating system and will be abstracted by an OS-agnostic class Window located in lexgine::core)
class Entity : public ErrorBehavioral
{
public:
    EntityID getId() const;    //! returns unique identifier of the entity

    std::string getStringName() const;	//! returns user-friendly string name of the entity
    void setStringName(std::string const& entity_string_name);	//! sets new user-friendly string name for the entity

    static uint64_t aliveEntities();    //! returns number of alive entities owned by the calling thread


    Entity();
    Entity(Entity const& other);
    Entity(Entity&& other);
    Entity& operator=(Entity const& other);
    Entity& operator=(Entity&& other);
    virtual ~Entity();


private:
    EntityID m_id;    //!< unique identifier of the entity
    std::string m_string_name;	//!< user-friendly string name of the entity
    static thread_local uint64_t m_alive_entities;    //!< number of alive entities owned by the calling thread
};


//! Template version of the Entity class that keeps track of user-friendly string name of inherited class.
//! Note that name should point to a C-string with external linkage
template<char const* name>
class NamedEntity : public Entity
{
public:
    //! adds information regarding construction of the named entity into the log
    NamedEntity()
    {
        logger().out(name + std::string{ " created, id=" } + getId().toString());
    }

    //! adds information regarding copy-construction of the named entity into the log
    NamedEntity(NamedEntity const& other) :
        Entity{ other }
    {
        logger().out(name + std::string{ " copied from object with id=" } + other.getId().toString()
            + " to new " + name + ". New id=" + getId().toString());
    }

    //! adds information regarding move-construction of the named entity into the log
    NamedEntity(NamedEntity&& other) :
        Entity{ std::move(other) }
    {
        logger().out(name + std::string{ " moved from object with id=" } + other.getId().toString()
            + " to new " + name + ". New id=" + getId().toString());
    }

    NamedEntity& operator=(NamedEntity const&) = default;
    NamedEntity& operator=(NamedEntity&&) = default;

    //! adds information regarding destruction of the named entity into the log
    virtual ~NamedEntity()
    {
        logger().out(name + std::string{ " with id=" } + getId().toString() + " destroyed");
    }

    //! returns user-friendly string of the named entity
    static constexpr char const* getMetaName() { return name; }
};



}}

#define LEXGINE_CORE_ENTITY_H
#endif