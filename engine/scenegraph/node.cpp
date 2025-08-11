#include "node.h"


namespace lexgine::scenegraph
{

Node::Node()
    : m_parent_to_local_transform{ 1.f }
    , m_local_to_parent_transform{ 1.f }
    , m_world_to_local_transform { 1.f }
    , m_local_to_world_transform { 1.f }
{

}

void Node::addChild(Node* child)
{
    m_children.push_back(child);
    child->m_parent = this;
    child->m_isDirty = true;
    child->m_parent_to_local_transform = child->m_local_to_parent_transform = lexgine::core::math::Matrix4f{ 1.f };
}

void Node::removeChild(Node* child)
{
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        m_children.erase(it);
        child->m_parent = nullptr;
        child->m_isDirty = true;
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

void Node::translate(lexgine::core::math::Vector3f& translation_vector)
{
    m_isDirty = true;
    lexgine::core::math::Matrix4f transform{
        lexgine::core::math::Vector4f{1.f, 0.f, 0.f, 0.f},
        lexgine::core::math::Vector4f{0.f, 1.f, 0.f, 0.f},
        lexgine::core::math::Vector4f { 0.f, 0.f, 1.f, 0.f },
        lexgine::core::math::Vector4f { translation_vector.x, translation_vector.y, translation_vector.z, 1.f }
    };

    lexgine::core::math::Matrix4f inverse_transform {
        lexgine::core::math::Vector4f { 1.f, 0.f, 0.f, 0.f },
        lexgine::core::math::Vector4f { 0.f, 1.f, 0.f, 0.f },
        lexgine::core::math::Vector4f { 0.f, 0.f, 1.f, 0.f },
        lexgine::core::math::Vector4f { -translation_vector.x, -translation_vector.y, -translation_vector.z, 1.f }
    };

    m_parent_to_local_transform = inverse_transform;
    m_local_to_parent_transform = transform;

}

void Node::rotate(lexgine::core::math::Vector3f& rotation_axis, float angle)
{
    lexgine::core::math::Matrix4f K{
        lexgine::core::math::Vector4f{0.f, rotation_axis.z, -rotation_axis.y, 0.f},
        lexgine::core::math::Vector4f{-rotation_axis.z, 0.f, rotation_axis.x, 0.f},
        lexgine::core::math::Vector4f{rotation_axis.y, -rotation_axis.x, 0.f, 0.f},
        lexgine::core::math::Vector4f{0.f, 0.f, 0.f, 1.f}
    };
    m_local_to_parent_transform = lexgine::core::math::Matrix4f{ 1.f } + std::sinf(angle) * K + (1.f - std::cosf(angle)) * (K * K);
    m_parent_to_local_transform = glm::inverse(m_local_to_parent_transform);
}

void Node::scale(lexgine::core::math::Vector3f& scaling_vector)
{
    m_isDirty = true;
    lexgine::core::math::Matrix4f transform {
        lexgine::core::math::Vector4f { scaling_vector.x, 0.f, 0.f, 0.f },
        lexgine::core::math::Vector4f { 0.f, scaling_vector.y, 0.f, 0.f },
        lexgine::core::math::Vector4f { 0.f, 0.f, scaling_vector.z, 0.f },
        lexgine::core::math::Vector4f { 0.f, 0.f, 0.f, 1.f }
    };
    lexgine::core::math::Matrix4f inverse_transform {
        lexgine::core::math::Vector4f { 1.f / scaling_vector.x, 0.f, 0.f, 0.f },
        lexgine::core::math::Vector4f { 0.f, 1.f / scaling_vector.y, 0.f, 0.f },
        lexgine::core::math::Vector4f { 0.f, 0.f, 1.f / scaling_vector.z, 0.f },
        lexgine::core::math::Vector4f { 0.f, 0.f, 0.f, 1.f }
    };
    m_parent_to_local_transform = inverse_transform;
    m_local_to_parent_transform = transform;
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

void Node::updateTransforms() const
{
    if (!m_isDirty)
    {
        return;
    }

    if (!m_parent)
    {
        m_parent_to_local_transform = m_local_to_parent_transform
            = m_world_to_local_transform = m_local_to_world_transform = lexgine::core::math::Matrix4f{ 1.f };
    }
    else
    {
        if (m_parent->m_isDirty)
        {
            m_parent->updateTransforms();
        }

        m_world_to_local_transform = m_parent->m_world_to_local_transform * m_parent_to_local_transform;
        m_local_to_world_transform = m_local_to_parent_transform * m_parent->m_local_to_world_transform;
    }
}

}

