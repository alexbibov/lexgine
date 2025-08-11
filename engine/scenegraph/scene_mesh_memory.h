#ifndef LEXGINE_SCENEGRAPH_SCENE_MESH_MEMORY_H
#define LEXGINE_SCENEGRAPH_SCENE_MESH_MEMORY_H

#include <vector>
#include <thread>
#include <queue>

#include "engine/core/globals.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/upload_buffer_allocator.h"
#include "engine/core/dx/d3d12/resource_data_uploader.h"

namespace lexgine::scenegraph
{

struct SceneMemoryBufferHandle
{
    size_t offset;
    size_t size;
};

class SceneMeshMemory
{
public:
    SceneMeshMemory(core::Globals& globals, uint64_t size);

    size_t size() const { return m_gpu_scene_memory_buffer.descriptor().width; }

    SceneMemoryBufferHandle addData(void const* p_data, size_t size);    //! Adds new data to be asynchronously uploaded to the scene mesh memory and returns a handle allowing to access it later
    void uploadAllData();    // upload all previously scheduled data
    
    void reset();

    bool isReady() const;
    void waitUntillReady() const;

    core::dx::d3d12::Resource getGPUResource() const { return m_gpu_scene_memory_buffer; };

private:
    core::dx::d3d12::CommittedResource m_gpu_scene_memory_buffer;
    core::dx::d3d12::DedicatedUploadDataStreamAllocator m_upload_data_stream_allocator;
    core::dx::d3d12::ResourceDataUploader m_resource_data_uploader;
    size_t m_data_upload_offset{ 0 };
};

}

#endif