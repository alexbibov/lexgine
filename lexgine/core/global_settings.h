#ifndef LEXGINE_CORE_GLOBAL_SETTINGS_H
#define LEXGINE_CORE_GLOBAL_SETTINGS_H

#include <cstdint>
#include <string>
#include <vector>

namespace lexgine { namespace core { 


//! Encapsulates global settings of the engine
class GlobalSettings
{
private:

    uint8_t m_number_of_workers;
    bool m_deferred_pso_compilation;
    bool m_deferred_shader_compilation;
    bool m_deferred_root_signature_compilation;
    std::vector<std::string> m_shader_lookup_directories;
    std::string m_cache_path;
    std::string m_combined_cache_name;
    uint64_t m_max_combined_cache_size;
    uint32_t m_descriptor_heaps_capacity;
    uint32_t m_upload_heap_capacity;
    bool m_enable_async_compute;
    bool m_enable_async_copy;
    uint16_t m_max_frames_in_flight;
    uint32_t m_max_non_blocking_upload_buffer_allocation_timeout;

    static uint32_t constexpr m_max_descriptors_per_page = 2048U;


public:
    GlobalSettings() = default;
    GlobalSettings(std::string const& json_settings_source_path);

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

    uint32_t getDescriptorPageCapacity(uint32_t page_id) const;
    uint32_t getDescriptorPageCount() const;
    uint32_t getUploadHeapCapacity() const;

    bool isAsyncComputeEnabled() const;
    bool isAsyncCopyEnabled() const;

    uint16_t getMaxFramesInFlight() const;

    uint32_t getMaxNonBlockingUploadBufferAllocationTimeout() const;

    
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
};

}}

#endif
