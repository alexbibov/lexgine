#ifndef LEXGINE_SCENEGRAPH_NODE_H
#define LEXGINE_SCENEGRAPH_NODE_H

#include <vector>
#include "lexgine_scenegraph_fwd.h"
#include "engine/core/math/matrix_types.h"
#include "engine/core/math/vector_types.h"
#include "engine/core/entity.h"

namespace lexgine::scenegraph {

class Node final : public core::NamedEntity<Node>
{
public:
    Node();
    Node(Node const&) = delete;
    Node(Node&& other) noexcept;
    ~Node() noexcept;

    Node& operator=(Node const&) = delete;
    Node& operator=(Node&& other) noexcept;

    Node* getParent() { return m_parent; }
    Node const* getParent() const { return const_cast<Node*>(this)->getParent(); }
    
    std::vector<Node*> const& children() const { return m_children; }
    void addChild(Node* child);
    void removeChild(Node* child);

    void addLod(Node* lod) { m_lods.push_back(lod); }
    Node const* getLod(size_t lod_index) const { return m_lods[lod_index]; }

    lexgine::core::math::Matrix4f const& parentToLocalTransform() const;
    lexgine::core::math::Matrix4f const& localToParentTransform() const;
    lexgine::core::math::Matrix4f const& worldToLocalTransform() const;
    lexgine::core::math::Matrix4f const& localToWorldTransform() const;
    lexgine::core::math::Vector4f const& worldPositionH() const;
    lexgine::core::math::Vector3f worldPosition() const;

    //! Sets the node's local translation, replacing the previous translation component.
    void setTranslation(lexgine::core::math::Vector3f const& translation_vector);

    //! Sets the node's local rotation to @p angle radians about @p rotation_axis. The axis is normalized internally.
    void setRotation(lexgine::core::math::Vector3f const& rotation_axis, float angle);

    //! Sets the node's local per-axis scale, replacing the previous scale component.
    void setScale(lexgine::core::math::Vector3f const& scaling_vector);

    void setLight(Light* light);
    void setCamera(Camera* camera);
    void setMesh(Mesh* mesh);

    Light* getLight() const { return m_light_ptr; }
    Camera* getCamera() const { return m_camera_ptr; }
    Mesh* getMesh() const { return m_mesh_ptr; }

private:
    void recomputeLocalTransform();
    void invalidateSubtree();
    void updateTransforms() const;

private:
    Light* m_light_ptr{ nullptr };
    Camera* m_camera_ptr{ nullptr };
    Mesh* m_mesh_ptr{ nullptr };

    Node* m_parent { nullptr };

    std::vector<Node*> m_lods;
    std::vector<Node*> m_children;

    lexgine::core::math::Vector3f m_translation{ 0.f, 0.f, 0.f };
    lexgine::core::math::Matrix4f m_rotation{ 1.f };
    lexgine::core::math::Vector3f m_scale{ 1.f, 1.f, 1.f };

    mutable bool m_is_dirty = true;
    lexgine::core::math::Matrix4f m_parent_to_local_transform;
    lexgine::core::math::Matrix4f m_local_to_parent_transform;
    mutable lexgine::core::math::Matrix4f m_world_to_local_transform;
    mutable lexgine::core::math::Matrix4f m_local_to_world_transform;
};

}

#endif
