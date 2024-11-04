#ifndef LEXGINE_SCENEGRAPH_SCENE_MESH_MEMORY_H
#define LEXGINE_SCENEGRAPH_SCENE_MESH_MEMORY_H

#include <vector>
#include <thread>
#include <queue>

#include "engine/core/globals.h"
#include "engine/core/dx/d3d12/resource.h"
#include "engine/core/dx/d3d12/upload_buffer_allocator.h"
#include "engine/core/dx/d3d12/resource_data_uploader.h"
#include "engine/core/dx/d3d12/resource.h"

namespace lexgine::scenegraph
{

struct MeshBufferHandle
{
    size_t offset;
    size_t size;
};

class SceneMeshMemory
{
public:
    SceneMeshMemory(core::Globals& globals, uint64_t size);

    size_t size() const { return m_gpu_scene_memory_buffer.descriptor().width; }

    MeshBufferHandle addData(void const* p_data, size_t size);    //! Adds new data to be asynchronously uploaded to the scene mesh memory and returns a handle allowing to access it later
    bool isReady() const;

    core::dx::d3d12::Resource getGPUResource() const { return m_gpu_scene_memory_buffer; };

private:
    void dataUploadWatchdog();

private:
    struct DataUploadRequestDesc
    {
        void const* p_source_data;
        size_t data_size;
        size_t upload_offset;
    };

private:
    core::Globals const& m_globals;
    core::dx::d3d12::CommittedResource m_gpu_scene_memory_buffer;
    core::dx::d3d12::DedicatedUploadDataStreamAllocator m_upload_data_stream_allocator;
    core::dx::d3d12::ResourceDataUploader m_resource_data_uploader;

    std::thread m_data_upload_worker;
    mutable std::mutex m_data_upload_mutex;
    size_t m_data_upload_offset{ 0 };
    std::queue<DataUploadRequestDesc> m_upload_request_queue;
};

}

#endif