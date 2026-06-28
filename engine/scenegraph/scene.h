#ifndef LEXGINE_SCENEGRAPH_SCENE_H
#define LEXGINE_SCENEGRAPH_SCENE_H

#include <filesystem>
#include <future>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include <tinygltf/tiny_gltf_v3.h>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/scenegraph/lexgine_scenegraph_fwd.h"
#include "engine/core/entity.h"
#include "engine/core/math/matrix_types.h"
#include "engine/core/math/vector_types.h"
#include "engine/core/misc/datetime.h"
#include "engine/core/misc/optional.h"
#include "engine/core/dx/d3d12/d3d12_tools.h"
#include "engine/core/dx/d3d12/pipeline_state.h"
#include "scene_mesh_memory.h"
#include "mesh.h"
#include "light.h"
#include "image.h"
#include "sampler.h"
#include "camera.h"
#include "material.h"

namespace lexgine::scenegraph
{

enum class SceneSource
{
    gltf,
    glb
};

class Scene : public core::NamedEntity<Scene>, public std::enable_shared_from_this<Scene>
{
public:
    static constexpr uint32_t c_invalid_id = std::numeric_limits<uint32_t>::max();

public:

#pragma region SceneNode
    //! Scene graph node. Owned by a Scene; refers to its parent, children, LODs and attached
    //! light/camera/mesh by uint32_t ids (indices into the owning Scene's vectors). An id equal
    //! to Scene::c_invalid_id denotes "none". Ids are stable across reallocation of the owning
    //! vectors, so the node is freely relocatable (move ctor/assignment are defaulted).
    class Node final : public core::NamedEntity<Node>
    {
    public:
        Node(Scene* owner, uint32_t self_id);
        Node(Node const&) = delete;
        Node(Node&&) noexcept = default;
        ~Node() noexcept = default;

        Node& operator=(Node const&) = delete;
        Node& operator=(Node&&) noexcept = default;

        uint32_t getSelfId() const { return m_self_id; }

        uint32_t getParentId() const { return m_parent_id; }
        std::vector<uint32_t> const& children() const { return m_children; }
        void addChild(uint32_t child_id);
        void removeChild(uint32_t child_id);

        void addLod(uint32_t lod_id) { m_lods.push_back(lod_id); }
        uint32_t getLod(size_t lod_index) const { return m_lods[lod_index]; }

        core::math::Matrix4f const& parentToLocalTransform() const;
        core::math::Matrix4f const& localToParentTransform() const;
        core::math::Matrix4f const& worldToLocalTransform() const;
        core::math::Matrix4f const& localToWorldTransform() const;
        core::math::Vector4f const& worldPositionH() const;
        core::math::Vector3f worldPosition() const;

        //! Sets the node's local translation, replacing the previous translation component.
        void setTranslation(core::math::Vector3f const& translation_vector);

        //! Sets the node's local rotation to @p angle radians about @p rotation_axis. The axis is normalized internally.
        void setRotation(core::math::Vector3f const& rotation_axis, float angle);

        //! Sets the node's local per-axis scale, replacing the previous scale component.
        void setScale(core::math::Vector3f const& scaling_vector);

        void setLight(uint32_t light_id);
        void setCamera(uint32_t camera_id);
        void setMesh(uint32_t mesh_id);

        uint32_t getLight() const { return m_light_id; }
        uint32_t getCamera() const { return m_camera_id; }
        uint32_t getMesh() const { return m_mesh_id; }

    private:
        void recomputeLocalTransform();
        void invalidateSubtree();
        void updateTransforms() const;

    private:
        Scene* m_owner{ nullptr };
        uint32_t m_self_id{ c_invalid_id };
        uint32_t m_light_id{ c_invalid_id };
        uint32_t m_camera_id{ c_invalid_id };
        uint32_t m_mesh_id{ c_invalid_id };
        uint32_t m_parent_id{ c_invalid_id };

        std::vector<uint32_t> m_lods;
        std::vector<uint32_t> m_children;

        core::math::Vector3f m_translation{ 0.f, 0.f, 0.f };
        core::math::Matrix4f m_rotation{ 1.f };
        core::math::Vector3f m_scale{ 1.f, 1.f, 1.f };

        mutable bool m_is_dirty = true;
        core::math::Matrix4f m_parent_to_local_transform;
        core::math::Matrix4f m_local_to_parent_transform;
        mutable core::math::Matrix4f m_world_to_local_transform;
        mutable core::math::Matrix4f m_local_to_world_transform;
    };

#pragma endregion SceneNode

public:
    static std::shared_ptr<Scene> loadScene(
        core::Globals& globals,
        core::dx::d3d12::BasicRenderingServices& basic_rendering_services,
        std::filesystem::path const& path_to_scene, unsigned scene_id
    );
    static std::shared_ptr<Scene> loadScene(
        core::Globals& globals,
        core::dx::d3d12::BasicRenderingServices& basic_rendering_services,
        std::filesystem::path const& path_to_scene,
        std::string const& scene_name
    );

    void updateGeometryTransforms();

    core::dx::d3d12::ConstantBufferDataMapper& getSceneConstants() { return *m_scene_parameters_data_mapper; }

    uint32_t getCameraId(core::misc::HashedString const& camera_name) const;
    uint32_t getLightId(core::misc::HashedString const& light_name) const;

    Node& getSceneNode(uint32_t node_id) { return m_scene_nodes[node_id]; }
    Node const& getSceneNode(uint32_t node_id) const { return m_scene_nodes[node_id]; }

    //! Detaches a node from the scene graph: removes it from its parent's child list and
    //! orphans its children (their parent becomes c_invalid_id). Does not reclaim the node's
    //! storage slot.
    void removeNode(uint32_t node_id);

    void setCurrentCamera(uint32_t camear_id);
    uint32_t getCurrentCamera() const { return m_current_camera_node_id; }

    SceneSource getSceneSource() const { return m_scene_source; }
    bool loadStatus() const;

private:
    static constexpr char const* c_khr_light_punctual_ext = "KHR_lights_punctual";
    static constexpr char const* c_ext_mesh_gpu_instancing = "EXT_mesh_gpu_instancing";

private:
    struct SceneMemory
    {
        std::unique_ptr<SceneMeshMemory> scene_memory_buffer;
        std::vector<SceneMemoryBufferHandle> m_scene_memory_handles;

        SceneMemoryBufferHandle getBuffer(size_t id) { return m_scene_memory_handles[id]; }
    };

    struct MaterialStaticStateCreateInfo
    {
        core::dx::d3d12::caches::HLSLShaderHandle vertex_shader;
        core::dx::d3d12::caches::HLSLShaderHandle hull_shader;
        core::dx::d3d12::caches::HLSLShaderHandle domain_shader;
        core::dx::d3d12::caches::HLSLShaderHandle geometry_shader;
        core::dx::d3d12::caches::HLSLShaderHandle pixel_shader;
        core::VertexAttributeSpecificationList vertex_data_format;
        size_t hash() const;
    };

    struct MaterialStaticStateCreateInfoHasher
    {
        size_t operator()(MaterialStaticStateCreateInfo const& value) const
        {
            return value.hash();
        }
    };

    struct MaterialStaticStateCreateInfoComparator
    {
        bool operator() (MaterialStaticStateCreateInfo const& lhs,
            MaterialStaticStateCreateInfo const& rhs) const
        {
            if (&lhs == &rhs) return true;

            bool test =
                lhs.vertex_shader == rhs.vertex_shader
                && lhs.hull_shader == rhs.hull_shader
                && lhs.domain_shader == rhs.domain_shader
                && lhs.geometry_shader == rhs.geometry_shader
                && lhs.pixel_shader == rhs.pixel_shader;
            if (!test) return false;

            if (lhs.vertex_data_format.size() != rhs.vertex_data_format.size())
            {
                return false;
            }

            for (size_t i = 0; i < lhs.vertex_data_format.size(); ++i)
            {
                auto const& lhs_va = lhs.vertex_data_format[i];
                auto const& rhs_va = rhs.vertex_data_format[i];

                if ((!lhs_va && rhs_va) || (lhs_va && !rhs_va))
                {
                    return false;
                }
                if (!lhs_va || !rhs_va)
                {
                    continue;
                }

                if (lhs_va->input_slot() != rhs_va->input_slot()
                    || lhs_va->offset() != rhs_va->offset()
                    || lhs_va->name_index() != rhs_va->name_index()
                    || lhs_va->instancingRate() != rhs_va->instancingRate()
                    || lhs_va->size() != rhs_va->size()
                    || lhs_va->capacity() != rhs_va->capacity()
                    || lhs_va->format<core::EngineApi::Direct3D12>() != rhs_va->format<core::EngineApi::Direct3D12>()
                    || lhs_va->name() != rhs_va->name())
                {
                    return false;
                }
            }

            return true;
        }
    };

    using MaterialStaticStateCreateInfoSet =
        std::unordered_set<MaterialStaticStateCreateInfo, MaterialStaticStateCreateInfoHasher, MaterialStaticStateCreateInfoComparator>;

    struct MaterialStaticStateHasher
    {
        size_t operator()(MaterialStaticState const& value) const
        {
            return value.pipelineDescriptor().hash()->fold();
        }
    };

    struct MaterialStaticStateComparator
    {
        bool operator()(MaterialStaticState const& lhs, MaterialStaticState const& rhs) const
        {
            core::dx::d3d12::GraphicsPSODescriptor const& pso_desc_lhs = lhs.pipelineDescriptor();
            core::dx::d3d12::GraphicsPSODescriptor const& pso_desc_rhs = rhs.pipelineDescriptor();
            return pso_desc_lhs == pso_desc_rhs;
        }
    };

    using MaterialStaticStateSet =
        std::unordered_set<MaterialStaticState, MaterialStaticStateHasher, MaterialStaticStateComparator>;

    struct MaterialAttachment
    {
        MaterialStaticStateCreateInfoSet::const_iterator create_info_it;
        MaterialStaticStateSet::const_iterator material_static_state_it;
        std::unordered_map<size_t, std::vector<size_t>> target_meshes;
    };

    enum class NodeDataType
    {
        light,
        camera,
        mesh,
        skin
    };
    struct NodeDataDesc
    {
        NodeDataType attached_data_type;
        int gltf_attachment_index;
    };

    //! Maps gltf vector indices to the corresponding in-scene storage indices
    struct GltfToSceneIndexMap
    {
        std::unordered_map<int, int> light_ids;
        std::unordered_map<int, int> material_ids;
        std::unordered_map<int, int> mesh_ids;
        std::unordered_map<int, int> camera_ids;
        std::unordered_map<int, int> animation_ids;
        std::unordered_map<int, int> buffer_ids;
        std::unordered_map<int, int> texture_ids;
        std::unordered_map<int, int> sampler_ids;
        std::unordered_map<int, std::vector<NodeDataDesc>> node_attachments;
    };

#pragma region DrawDataMemory
    struct PerInstanceGpuData
    {
        core::math::Matrix4f transform;
    };

    struct PerInstanceCpuData
    {
        uint32_t owning_node_id;
        uint32_t draw_id;
    };

    struct DrawInstanceId
    {
        uint32_t mesh_id;
        uint32_t submesh_id;
        bool operator==(DrawInstanceId const&) const = default;
    };

    struct DrawInstanceIdHahser
    {
        size_t operator()(DrawInstanceId const& value) const
        {
            return static_cast<size_t>(value.mesh_id) ^ std::rotl(static_cast<size_t>(value.submesh_id), 17);
        }
    };

    struct Draw
    {
        uint32_t draw_query_id;
        DrawInstanceId draw_instance_id;
        std::vector<uint32_t> instance_indices;
    };

    struct DrawQuery
    {
        uint32_t material_id;
        std::vector<uint32_t> draw_ids;
    };
#pragma endregion DrawDataMemory

private:
    Scene(
        core::Globals& globals,
        core::dx::d3d12::BasicRenderingServices& basic_rendering_services,
        std::filesystem::path const& path_to_scene,
        unsigned scene_id
    );
    Scene(
        core::Globals& globals,
        core::dx::d3d12::BasicRenderingServices& basic_rendering_services,
        std::filesystem::path const& path_to_scene,
        std::string const& scene_name
    );

    std::optional<tinygltf3::Model> readGltfModel(std::filesystem::path const& path);
    bool readScene(tg3_model const& model, unsigned scene_index);

    int readSceneNode(
        tg3_model const& model,
        tg3_node const& node,
        GltfToSceneIndexMap& index_map
    );

    bool loadLights(
        tg3_model const& model,
        GltfToSceneIndexMap& index_map
    );
    bool loadTextures(
        tg3_model const& model,
        GltfToSceneIndexMap& index_map
    );
    bool loadMeshes(
        tg3_model const& model,
        GltfToSceneIndexMap& index_map
    );
    bool loadCameras(
        tg3_model const& model,
        GltfToSceneIndexMap& index_map
    );
    bool loadAnimations(
        tg3_model const& model,
        GltfToSceneIndexMap& index_map
    );

    MaterialStaticStateCreateInfoSet::const_iterator
        registerMaterialStaticState(
            tg3_material const& gltf_material,
            const lexgine::core::VertexAttributeSpecificationList& vertex_attributes
        );

    void buildDraws();

private:
    core::Globals& m_globals;
    core::dx::d3d12::BasicRenderingServices& m_basic_rendering_services;
    core::GlobalSettings& m_global_settings;
    core::misc::DateTime const m_timestamp;
    std::filesystem::path m_scene_path;
    SceneSource m_scene_source;
    int m_scene_index{ -1 };
    bool m_scene_source_parse_status{ false };
    std::unordered_map<std::string, bool> m_enabled_extensions = { {c_khr_light_punctual_ext, false} };
    
    std::vector<Node> m_scene_nodes;
    std::vector<Light> m_lights;
    std::vector<Texture> m_textures;
    std::vector<Sampler> m_samplers;
    std::vector<Material> m_materials;
    std::vector<Camera> m_cameras;
    std::vector<Mesh> m_scene_meshes;

    std::unordered_map<core::misc::HashedString, size_t> m_camera_names_lut;
    std::unordered_map<core::misc::HashedString, size_t> m_light_names_lut;

    SceneMemory m_scene_memory;
    MaterialStaticStateCreateInfoSet m_material_static_state_create_infos;
    MaterialStaticStateSet m_material_static_states;
    std::unordered_map<size_t, MaterialAttachment> m_material_attachements;

    uint32_t m_current_camera_node_id{ 0 };
    core::math::Vector3f m_current_camera_position;
    std::unique_ptr<core::dx::d3d12::ConstantBufferDataMapper> m_scene_parameters_data_mapper;

    size_t m_total_submesh_count{ 0 };

#pragma region DrawDataMemory
    std::unordered_map<uint32_t, uint32_t> m_material_to_draw_query_lut;  // maps material id to draw query id
    std::unordered_map<DrawInstanceId, uint32_t, DrawInstanceIdHahser> m_draw_instance_id_to_draw_lut;  // maps draw instance id to draw id
    std::vector<DrawQuery> m_draw_queries;
    std::vector<Draw> m_draws;
    std::vector<PerInstanceCpuData> m_instances_cpu_data;
    std::vector<PerInstanceGpuData> m_instances_gpu_data;
#pragma endregion DrawDataMemory
};

}

#endif
