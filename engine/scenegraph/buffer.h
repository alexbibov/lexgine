#ifndef LEXGINE_SCENEGRAPH_BUFFER_H

#include <vector>
#include <thread>

#include "engine/core/globals.h"
#include "engine/core/dx/d3d12/resource.h"
#include "engine/core/dx/d3d12/upload_buffer_allocator.h"
#include "engine/core/dx/d3d12/resource_data_uploader.h"

namespace lexgine::scenegraph
{

class Buffer
{
public:
    Buffer(core::Globals& globals, uint64_t size);

    size_t size() const { return m_buffer_gpu_memory_heap.capacity(); }

    uint64_t fill(void* p_src_data, size_t src_data_size_in_bytes);

private:
    void startWatchdog();
    void watchDelayedUploads();

private:
    core::Globals const& m_globals;
    core::dx::d3d12::Heap m_buffer_gpu_memory_heap;
    core::dx::d3d12::DedicatedUploadDataStreamAllocator m_upload_data_stream_allocator;
    core::dx::d3d12::ResourceDataUploader m_resource_data_uploader;

    std::thread m_uploading_watchdog;
    std::vector<uint8_t> m_intermediate_buffer;
    std::mutex m_intermediate_buffer_mutex;
};

}

#define LEXGINE_SCENEGRAPH_BUFFER_H
#endif