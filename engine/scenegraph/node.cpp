#include "node.h"

#include <algorithm>
#include <cmath>
#include <utility>


namespace lexgine::scenegraph
{

Node::Node()
    : m_parent_to_local_transform{ 1.f }
    , m_local_to_parent_transform{ 1.f }
    , m_world_to_local_transform { 1.f }
    , m_local_to_world_transform { 1.f }
{

}

Node::Node(Node&& other) noexcept
    : core::NamedEntity<Node>{ std::move(other)}
    , m_light_ptr{ other.m_light_ptr }
    , m_camera_ptr{ other.m_camera_ptr }
    , m_mesh_ptr{ other.m_mesh_ptr }
    , m_translation{ other.m_translation }
    , m_rotation{ other.m_rotation }
    , m_scale{ other.m_scale }
    , m_parent_to_local_transform{ other.m_parent_to_local_transform }
    , m_local_to_parent_transform{ other.m_local_to_parent_transform }
    , m_world_to_local_transform{ other.m_world_to_local_transform }
    , m_local_to_world_transform{ other.m_local_to_world_transform }
{
    std::swap(m_lods, other.m_lods);
    if (Node* parent = other.m_parent)
    {
        parent->removeChild(&other);
        parent->addChild(this);
    }
    std::swap(m_children, other.m_children);
    for (Node* n : m_children)
    {
        n->m_parent = this;
    }
    invalidateSubtree();
}

Node::~Node() noexcept
{
    if (m_parent)
    {
        m_parent->removeChild(this);
        m_parent = nullptr;
    }
    for (Node* n : m_children)
    {
        n->m_parent = nullptr;
        n->invalidateSubtree();
    }
}

Node& Node::operator=(Node&& other) noexcept
{
    if (this == &other)
        return *this;

    if (m_parent)
    {
        m_parent->removeChild(this);
    }
    for (Node* n : m_children)
    {
        n->m_parent = nullptr;
        n->invalidateSubtree();
    }
    m_children.clear();

    core::NamedEntity<Node>::operator=(std::move(other));

    m_light_ptr = other.m_light_ptr;
    m_camera_ptr = other.m_camera_ptr;
    m_mesh_ptr = other.m_mesh_ptr;
    m_translation = other.m_translation;
    m_rotation = other.m_rotation;
    m_scale = other.m_scale;
    m_parent_to_local_transform = other.m_parent_to_local_transform;
    m_local_to_parent_transform = other.m_local_to_parent_transform;
    m_world_to_local_transform = other.m_world_to_local_transform;
    m_local_to_world_transform = other.m_local_to_world_transform;

    m_lods = std::move(other.m_lods);

    if (Node* parent = other.m_parent)
    {
        parent->removeChild(&other);
        parent->addChild(this);
    }

    m_children = std::move(other.m_children);
    for (Node* n : m_children)
    {
        n->m_parent = this;
    }

    invalidateSubtree();

    return *this;
}

void Node::addChild(Node* child)
{
    if (child->m_parent)
    {
        child->m_parent->removeChild(child);
    }

    m_children.push_back(child);
    child->m_parent = this;

    if (!child->m_is_dirty)
    {
        child->invalidateSubtree();
    }
}

void Node::removeChild(Node* child)
{
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        m_children.erase(it);
        child->m_parent = nullptr;
        child->invalidateSubtree();
    }
}

lexgine::core::math::Matrix4f const& Node::parentToLocalTransform() const
{
    updateTransforms();
    return m_parent_to_local_transform;
}

lexgine::core::math::Matrix4f const& Node::localToParentTransform() const
{
    updateTransforms();
    return m_local_to_parent_transform;
}

lexgine::core::math::Matrix4f const& Node::worldToLocalTransform() const
{
    updateTransforms();
    return m_world_to_local_transform;
}

lexgine::core::math::Matrix4f const& Node::localToWorldTransform() const
{
    updateTransforms();
    return m_local_to_world_transform;
}

lexgine::core::math::Vector4f const& Node::worldPositionH() const
{
    updateTransforms();
    return m_local_to_world_transform[3];
}

lexgine::core::math::Vector3f Node::worldPosition() const
{
    auto const& position_h = worldPositionH();
    return lexgine::core::math::Vector3f{ position_h.x, position_h.y, position_h.z };
}

void Node::setTranslation(lexgine::core::math::Vector3f const& translation_vector)
{
    m_translation = translation_vector;
    recomputeLocalTransform();
    invalidateSubtree();
}

void Node::setRotation(lexgine::core::math::Vector3f const& rotation_axis, float angle)
{
    float const axis_length = std::sqrt(
        rotation_axis.x * rotation_axis.x +
        rotation_axis.y * rotation_axis.y +
        rotation_axis.z * rotation_axis.z);

    if (axis_length < 1e-6f)
    {
        m_rotation = lexgine::core::math::Matrix4f{ 1.f };
    }
    else
    {
        float const nx = rotation_axis.x / axis_length;
        float const ny = rotation_axis.y / axis_length;
        float const nz = rotation_axis.z / axis_length;

        lexgine::core::math::Matrix4f K{
            lexgine::core::math::Vector4f{ 0.f, nz, -ny, 0.f },
            lexgine::core::math::Vector4f{ -nz, 0.f, nx, 0.f },
            lexgine::core::math::Vector4f{ ny, -nx, 0.f, 0.f },
            lexgine::core::math::Vector4f{ 0.f, 0.f, 0.f, 1.f }
        };
        m_rotation = lexgine::core::math::Matrix4f{ 1.f } + std::sin(angle) * K + (1.f - std::cos(angle)) * (K * K);
    }

    recomputeLocalTransform();
    invalidateSubtree();
}

void Node::setScale(lexgine::core::math::Vector3f const& scaling_vector)
{
    m_scale = scaling_vector;
    recomputeLocalTransform();
    invalidateSubtree();
}

void Node::recomputeLocalTransform()
{
    lexgine::core::math::Matrix4f scale_transform{
        lexgine::core::math::Vector4f{ m_scale.x, 0.f, 0.f, 0.f },
        lexgine::core::math::Vector4f{ 0.f, m_scale.y, 0.f, 0.f },
        lexgine::core::math::Vector4f{ 0.f, 0.f, m_scale.z, 0.f },
        lexgine::core::math::Vector4f{ 0.f, 0.f, 0.f, 1.f }
    };

    lexgine::core::math::Matrix4f translation_transform{
        lexgine::core::math::Vector4f{ 1.f, 0.f, 0.f, 0.f },
        lexgine::core::math::Vector4f{ 0.f, 1.f, 0.f, 0.f },
        lexgine::core::math::Vector4f{ 0.f, 0.f, 1.f, 0.f },
        lexgine::core::math::Vector4f{ m_translation.x, m_translation.y, m_translation.z, 1.f }
    };

    m_local_to_parent_transform = translation_transform * m_rotation * scale_transform;
    m_parent_to_local_transform = glm::inverse(m_local_to_parent_transform);
}

void Node::setLight(Light* light)
{
    m_light_ptr = light;
    m_camera_ptr = nullptr;
    m_mesh_ptr = nullptr;
}

void Node::setCamera(Camera* camera)
{
    m_camera_ptr = camera;
    m_light_ptr = nullptr;
    m_mesh_ptr = nullptr;
}

void Node::setMesh(Mesh* mesh)
{
    m_mesh_ptr = mesh;
    m_camera_ptr = nullptr;
    m_light_ptr = nullptr;
}

void Node::invalidateSubtree()
{
    m_is_dirty = true;
    for (Node* child : m_children)
    {
        child->invalidateSubtree();
    }
}

void Node::updateTransforms() const
{
    if (!m_is_dirty)
    {
        return;
    }

    if (!m_parent)
    {
        m_world_to_local_transform = m_parent_to_local_transform;
        m_local_to_world_transform = m_local_to_parent_transform;
    }
    else
    {
        m_parent->updateTransforms();

        m_world_to_local_transform = m_parent_to_local_transform * m_parent->m_world_to_local_transform;
        m_local_to_world_transform = m_parent->m_local_to_world_transform * m_local_to_parent_transform;
    }

    m_is_dirty = false;
}

}

