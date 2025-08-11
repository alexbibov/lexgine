#ifndef LEXGINE_SCENEGRAPH_NODE_H
#define LEXGINE_SCENEGRAPH_NODE_H

#include <vector>
#include "lexgine_scenegraph_fwd.h"
#include "engine/core/math/matrix_types.h"
#include "engine/core/math/vector_types.h"
#include "engine/core/entity.h"
#include "class_names.h"

namespace lexgine::scenegraph {

class Node : public core::NamedEntity<class_names::Node>
{
public:
    Node();

    Node(Node const&) = delete;
    Node(Node&&) = default;

    Node& operator=(Node const&) = delete;
    Node& operator=(Node&&) = default;

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

    void translate(lexgine::core::math::Vector3f& translation_vector);
    void rotate(lexgine::core::math::Vector3f& rotation_axis, float angle);
    void scale(lexgine::core::math::Vector3f& scaling_vector);

    void setLight(Light* light);
    void setCamera(Camera* camera);
    void setMesh(Mesh* mesh);

    Light* getLight() const { return m_light_ptr; }
    Camera* getCamera() const { return m_camera_ptr; }
    Mesh* getMesh() const { return m_mesh_ptr; }

private:
    void updateTransforms() const;

private:
    Light* m_light_ptr{ nullptr };
    Camera* m_camera_ptr{ nullptr };
    Mesh* m_mesh_ptr{ nullptr };

    Node* m_parent { nullptr };

    std::vector<Node*> m_lods;
    std::vector<Node*> m_children;

    bool m_isDirty = true;
    mutable lexgine::core::math::Matrix4f m_parent_to_local_transform;
    mutable lexgine::core::math::Matrix4f m_local_to_parent_transform;
    mutable lexgine::core::math::Matrix4f m_world_to_local_transform;
    mutable lexgine::core::math::Matrix4f m_local_to_world_transform;
};

}

#endif