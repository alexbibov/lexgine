#ifndef LEXGINE_CORE_GLOBAL_SETTINGS_H
#define LEXGINE_CORE_GLOBAL_SETTINGS_H

#include <cstdint>
#include <string>
#include <vector>
#include <array>

#include "engine/core/dx/d3d12/descriptor_heap.h"

namespace lexgine::core { 


//! Encapsulates global settings of the engine
class GlobalSettings
{
public:
    GlobalSettings() = default;
    GlobalSettings(std::string const& json_settings_source_path, 
        int8_t time_zone, bool dts);

    int8_t getTimeZone() const;    //! retrieves the time zone of the host
    bool isDTS() const;    //! identifies, whether the time zone, in which the host is running is using daylight time saving

    void serialize(std::string const& json_serialization_path) const;

    // *** the following functions are used to retrieve global settings ***

    uint8_t getNumberOfWorkers() const;
    bool isDeferredShaderCompilationOn() const;
    bool isDeferredPSOCompilationOn() const;
    bool isDeferredRootSignatureCompilationOn() const;
    std::vector<std::string> const& getShaderLookupDirectories() const;
    std::string getCacheDirectory() const;
    std::string getCombinedCacheName() const;
    uint64_t getMaxCombinedCacheSize() const;
    uint64_t getMaxCombinedTextureCacheSize() const;

    uint32_t getDescriptorHeapPageCapacity(dx::d3d12::DescriptorHeapType descriptor_heap_type) const;
    uint32_t getDescriptorHeapPageCount(dx::d3d12::DescriptorHeapType descriptor_heap_type) const;
    uint32_t getUploadHeapCapacity() const;
    size_t getStreamedConstantDataPartitionSize() const;    //! returns size of upload buffer partition dedicated to constant data streaming
    size_t getStreamedGeometryDataPartitionSize() const;    //! returns size of upload buffer partition dedicated to dynamic geometry data streaming
    size_t getTextureUploadPartitionSize() const;    //! returns size of upload buffer partition dedicated to texture uploading

    bool isAsyncComputeEnabled() const;
    bool isAsyncCopyEnabled() const;
    bool isProfilingEnabled() const;
    bool isCacheEnabled() const;

    uint16_t getMaxFramesInFlight() const;

    uint32_t getMaxNonBlockingUploadBufferAllocationTimeout() const;

    bool isGpuAcceleratedTextureConversionEnabled() const;


    // *** the following functions are used to alter the global settings during run time. All functions return 'true' in case of success and 'false' if the parameter's value cannot be changed ***

    void setNumberOfWorkers(uint8_t num_workers);
    void setIsDeferredShaderCompilationOn(bool is_enabled);
    void setIsDeferredPSOCompilationOn(bool is_enabled);
    void setIsDeferredRootSignatureCompilationOn(bool is_enabled);
    void addShaderLookupDirectory(std::string const& path);
    void clearShaderLookupDirectories();
    void setCacheDirectory(std::string const& path);
    void setCacheName(std::string const& name);
    void setIsAsyncComputeEnabled(bool is_enabled);
    void setIsAsyncCopyEnabled(bool is_enabled);
    void setIsProfilingEnabled(bool is_enabled);


private:
    int8_t m_time_zone;
    bool m_dts;

    uint8_t m_number_of_workers;
    bool m_deferred_pso_compilation;
    bool m_deferred_shader_compilation;
    bool m_deferred_root_signature_compilation;
    std::vector<std::string> m_shader_lookup_directories;
    std::string m_cache_path;
    std::string m_combined_cache_name;
    uint64_t m_max_combined_cache_size;
    uint64_t m_max_combined_texture_cache_size;
    uint32_t m_upload_heap_capacity;
    float m_streamed_constant_data_partitioning;
    float m_streamed_geometry_data_partitioning;
    bool m_enable_async_compute;
    bool m_enable_async_copy;
    uint16_t m_max_frames_in_flight;
    uint32_t m_max_non_blocking_upload_buffer_allocation_timeout;
    bool m_enable_profiling;
    bool m_enable_cache;
    bool m_enable_gpu_accelerated_texture_conversion;

    std::array<uint32_t, static_cast<size_t>(dx::d3d12::DescriptorHeapType::count)> m_descriptors_per_page;
    std::array<uint32_t, static_cast<size_t>(dx::d3d12::DescriptorHeapType::count)> m_descriptor_heap_page_count;
};

}

#endif
