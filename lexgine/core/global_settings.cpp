#include "global_settings.h"
#include "initializer.h"

#include "lexgine/core/build_info.h"
#include "lexgine/core/misc/misc.h"
#include "lexgine/core/misc/log.h"


#include "3rd_party/json/json.hpp"
#include <fstream>


using namespace lexgine::core;
using json = nlohmann::json;


namespace {

template<typename T>
std::string to_string(T const& value) { return std::to_string(value); }

template<>
std::string to_string<std::string>(std::string const& value) { return value; }

}


GlobalSettings::GlobalSettings(std::string const& json_settings_source_path)
{
    // initialize default values for the settings
    {
        m_number_of_workers = 8U;

        m_deferred_shader_compilation = true;
        m_deferred_pso_compilation = true;
        m_deferred_root_signature_compilation = true;

        m_cache_path = std::string{ PROJECT_CODE_NAME } +"__v." + std::to_string(PROJECT_VERSION_MAJOR) + "."
            + std::to_string(PROJECT_VERSION_MINOR) + "__rev." + std::to_string(PROJECT_VERSION_REVISION)
            + "__" + PROJECT_VERSION_STAGE + "__cache/";

        m_combined_cache_name = std::string{ PROJECT_CODE_NAME } +"__v." + std::to_string(PROJECT_VERSION_MAJOR) + "."
            + std::to_string(PROJECT_VERSION_MINOR) + "__rev." + std::to_string(PROJECT_VERSION_REVISION)
            + "__" + PROJECT_VERSION_STAGE + ".combined_cache";

        m_max_combined_cache_size = 1024ull * 1024 * 1024 * 4;    // defaults to 4Gbs
        
        m_upload_heap_capacity = 1024 * 1024 * 256;    // 256MBs by default
        m_enable_async_compute = true;
        m_enable_async_copy = true;
        m_max_frames_in_flight = 6;
        m_max_non_blocking_upload_buffer_allocation_timeout = 1000U;


        {
            // Descriptor heaps total and per-page capacity default settings

            m_descriptors_per_page[static_cast<size_t>(DescriptorHeapType::cbv_srv_uav)] = 8192;
            m_descriptor_heap_page_count[static_cast<size_t>(DescriptorHeapType::cbv_srv_uav)] = 1;

            m_descriptors_per_page[static_cast<size_t>(DescriptorHeapType::sampler)] = 256;
            m_descriptor_heap_page_count[static_cast<size_t>(DescriptorHeapType::sampler)] = 1;

            m_descriptors_per_page[static_cast<size_t>(DescriptorHeapType::rtv)] = 512;
            m_descriptor_heap_page_count[static_cast<size_t>(DescriptorHeapType::rtv)] = 1;

            m_descriptors_per_page[static_cast<size_t>(DescriptorHeapType::dsv)] = 512;
            m_descriptor_heap_page_count[static_cast<size_t>(DescriptorHeapType::dsv)] = 1;
        }
    }


    misc::Optional<std::string> source_json = misc::readAsciiTextFromSourceFile(json_settings_source_path);
    
    if (!source_json.isValid())
    {
        misc::Log::retrieve()->out("WARNING: unable to parse global settings JSON file located at \"" 
            + json_settings_source_path + "\". The system will fall back to the default settings",
            misc::LogMessageType::exclamation);
        return;
    }


    try {
        json document = json::parse(static_cast<std::string&>(source_json));

        json::iterator p;


        auto yield_warning_log_message = [&json_settings_source_path](std::string const& setting_name, auto setting_default_value)
        {
            misc::Log::retrieve()->out("WARNING: unable to get value for \"" + setting_name + "\" from the settings file located at \""
                + json_settings_source_path + "\". The system will fall back to the default value \""
                + setting_name + " = " + to_string(setting_default_value) + "\"", misc::LogMessageType::exclamation);
        };


        if ((p = document.find("number_of_workers")) != document.end()
            && p->is_number_unsigned())
        {
            m_number_of_workers = *p;
        }
        else
        {
            yield_warning_log_message("number_of_workers", m_number_of_workers);
        }


        // Deferred pipeline compilation routine settings
        {
            if ((p = document.find("deferred_pso_compilation")) != document.end()
                && p->is_boolean())
            {
                m_deferred_pso_compilation = *p;
            }
            else
            {
                yield_warning_log_message("deferred_pso_compilation", m_deferred_pso_compilation);
            }

            if ((p = document.find("deferred_shader_compilation")) != document.end()
                && p->is_boolean())
            {
                m_deferred_shader_compilation = *p;
            }
            else
            {
                m_deferred_shader_compilation = m_deferred_pso_compilation;
                yield_warning_log_message("deferred_shader_compilation", m_deferred_shader_compilation);
            }

            if ((p = document.find("deferred_root_signature_compilation")) != document.end()
                && p->is_boolean())
            {
                m_deferred_root_signature_compilation = *p;
            }
            else
            {
                m_deferred_root_signature_compilation = m_deferred_pso_compilation;
                yield_warning_log_message("deferred_root_signature_compilation", m_deferred_root_signature_compilation);
            }
        }

        
        if ((p = document.find("shader_lookup_directories")) != document.end())
        {
            if (p->is_array())
            {
                for (auto& e : *p)
                {
                    if (e.is_string())
                    {
                        m_shader_lookup_directories.push_back(e);
                    }
                    else
                    {
                        misc::Log::retrieve()->out("WARNING: unable retrieve a value from JSON array \"shader_lookup_directories\" in the settings file located at \""
                            + json_settings_source_path + "\"; \"shader_lookup_directories\" is expected to be an array of strings but some of its elements appear to have non-string format. "
                            "Non-string elements will be ignored", misc::LogMessageType::exclamation);
                    }
                }
            }
            else
            {
                misc::Log::retrieve()->out("WARNING: unable to get value for \"shader_lookup_directories\" from the settings file located at \""
                    + json_settings_source_path + "\"; \"shader_lookup_directories\" is expected to be an array of strings but turned out to have different format."
                    " The system will search for shaders in the current working directory",
                    misc::LogMessageType::exclamation);
            }
        }
        
        if ((p = document.find("cache_path")) != document.end()
            && p->is_string())
        {
            m_cache_path = p->get<std::string>();
        }
        else
        {
            yield_warning_log_message("cache_path", m_cache_path);
        }

        if ((p = document.find("combined_cache_name")) != document.end()
            && p->is_string())
        {
            m_combined_cache_name = p->get<std::string>();
        }
        else
        {
            yield_warning_log_message("combined_cache_name", m_combined_cache_name);
        }

        if ((p = document.find("maximal_combined_cache_size")) != document.end()
            && p->is_number_unsigned())
        {
            m_max_combined_cache_size = p->get<uint64_t>();
        }
        else
        {
            yield_warning_log_message("maximal_combined_cache_size", 
                std::to_string(m_max_combined_cache_size / 1024 / 1024 / 1024) + "GBs");
        }

        if ((p = document.find("upload_heap_capacity")) != document.end()
            && p->is_number_unsigned())
        {
            m_upload_heap_capacity = p->get<uint32_t>();
        }
        else
        {
            yield_warning_log_message("upload_heap_capacity", 
                std::to_string(m_upload_heap_capacity / 1024 / 1024) + "MBs");
        }

        if ((p = document.find("enable_async_compute")) != document.end()
            && p->is_boolean())
        {
            m_enable_async_compute = p->get<bool>();
        }
        else
        {
            yield_warning_log_message("enable_async_compute", m_enable_async_compute);
        }

        if ((p = document.find("enable_async_copy")) != document.end()
            && p->is_boolean())
        {
            m_enable_async_copy = p->get<bool>();
        }
        else
        {
            yield_warning_log_message("enable_async_copy", m_enable_async_copy);
        }

        if ((p = document.find("max_frames_in_flight")) != document.end()
            && p->is_number_unsigned())
        {
            m_max_frames_in_flight = p->get<uint16_t>();
        }
        else
        {
            yield_warning_log_message("max_frames_in_flight", m_max_frames_in_flight);
        }

        if ((p = document.find("max_non_blocking_upload_buffer_allocation_timeout")) != document.end()
            && p->is_number_unsigned())
        {
            m_max_non_blocking_upload_buffer_allocation_timeout = p->get<uint32_t>();
        }
        else
        {
            yield_warning_log_message("max_non_blocking_upload_buffer_allocation_timeout",
                m_max_non_blocking_upload_buffer_allocation_timeout);
        }


        {
            // Descriptor heaps total and per-page capacity settings

            if ((p = document.find("resource_view_descriptors_per_page")) != document.end()
                && p->is_number_unsigned())
            {
                m_descriptors_per_page[static_cast<size_t>(DescriptorHeapType::cbv_srv_uav)] = p->get<uint32_t>();
            }
            else
            {
                yield_warning_log_message("resource_view_descriptors_per_page",
                    m_descriptors_per_page[static_cast<size_t>(DescriptorHeapType::cbv_srv_uav)]);
            }

            if ((p = document.find("resource_view_descriptor_heap_page_count")) != document.end()
                && p->is_number_unsigned())
            {
                m_descriptor_heap_page_count[static_cast<size_t>(DescriptorHeapType::cbv_srv_uav)] = p->get<uint32_t>();
            }
            else
            {
                yield_warning_log_message("resource_view_descriptor_heap_page_count",
                    m_descriptor_heap_page_count[static_cast<size_t>(DescriptorHeapType::cbv_srv_uav)]);
            }


            if ((p = document.find("sampler_descriptors_per_page")) != document.end()
                && p->is_number_unsigned())
            {
                m_descriptors_per_page[static_cast<size_t>(DescriptorHeapType::sampler)] = p->get<uint32_t>();
            }
            else
            {
                yield_warning_log_message("sampler_descriptors_per_page",
                    m_descriptors_per_page[static_cast<size_t>(DescriptorHeapType::sampler)]);
            }

            if ((p = document.find("sampler_descriptor_heap_page_count")) != document.end()
                && p->is_number_unsigned())
            {
                m_descriptor_heap_page_count[static_cast<size_t>(DescriptorHeapType::sampler)] = p->get<uint32_t>();
            }
            else
            {
                yield_warning_log_message("sampler_descriptor_heap_page_count",
                    m_descriptor_heap_page_count[static_cast<size_t>(DescriptorHeapType::sampler)]);
            }


            if ((p = document.find("render_target_view_descriptors_per_page")) != document.end()
                && p->is_number_unsigned())
            {
                m_descriptors_per_page[static_cast<size_t>(DescriptorHeapType::rtv)] = p->get<uint32_t>();
            }
            else
            {
                yield_warning_log_message("render_target_view_descriptors_per_page",
                    m_descriptors_per_page[static_cast<size_t>(DescriptorHeapType::rtv)]);
            }

            if ((p = document.find("render_target_view_descriptor_heap_page_count")) != document.end()
                && p->is_number_unsigned())
            {
                m_descriptor_heap_page_count[static_cast<size_t>(DescriptorHeapType::rtv)] = p->get<uint32_t>();
            }
            else
            {
                yield_warning_log_message("render_target_view_descriptor_heap_page_count",
                    m_descriptor_heap_page_count[static_cast<size_t>(DescriptorHeapType::rtv)]);
            }


            if ((p = document.find("depth_stencil_view_descriptors_per_page")) != document.end()
                && p->is_number_unsigned())
            {
                m_descriptors_per_page[static_cast<size_t>(DescriptorHeapType::dsv)] = p->get<uint32_t>();
            }
            else
            {
                yield_warning_log_message("depth_stencil_view_descriptors_per_page",
                    m_descriptors_per_page[static_cast<size_t>(DescriptorHeapType::dsv)]);
            }

            if ((p = document.find("depth_stencil_view_descriptor_heap_page_count")) != document.end()
                && p->is_number_unsigned())
            {
                m_descriptor_heap_page_count[static_cast<size_t>(DescriptorHeapType::dsv)] = p->get<uint32_t>();
            }
            else
            {
                yield_warning_log_message("depth_stencil_view_descriptor_heap_page_count",
                    m_descriptor_heap_page_count[static_cast<size_t>(DescriptorHeapType::dsv)]);
            }
        }
        
    }
    catch (...)
    {
        misc::Log::retrieve()->out("WARNING: JSON file located at \"" + json_settings_source_path + "\" has invalid format. The system will fall back to default settings",
            misc::LogMessageType::exclamation);
    }



    if (!m_deferred_pso_compilation)
    {
        // if deferred pso compilation is disabled then deferred shader and deferred root signature 
        // compilation tasks should also be compiled in immediate mode

        if (m_deferred_shader_compilation)
        {
            misc::Log::retrieve()->out("WARNING: deferred PSO compilation is disabled but deferred shader compilation that PSO compilation relies upon is switched on. "
                "Deferred shader compilation will therefore be force disabled", misc::LogMessageType::exclamation);
            m_deferred_shader_compilation = false;
        }

        if (m_deferred_root_signature_compilation)
        {
            misc::Log::retrieve()->out("WARNING: deferred PSO compilation is disabled but deferred root signature compilation that PSO compilation relies upon is switched on. "
                "Deferred root signature compilation will therefore be force disabled", misc::LogMessageType::exclamation);
            m_deferred_root_signature_compilation = false;
        }
    }
}

void GlobalSettings::serialize(std::string const& json_serialization_path) const
{
    std::ofstream ofile{ json_serialization_path, std::ios::out };
    if (!ofile)
    {
        misc::Log::retrieve()->out("WARNING: unable to establish output stream for the destination at \"" + json_serialization_path + "\". The current settings will not be serialized",
            misc::LogMessageType::exclamation);
        return;
    }

    json j = {
        { "number_of_workers", m_number_of_workers},
        { "deferred_shader_compilation", m_deferred_shader_compilation},
        { "deferred_pso_compilation", m_deferred_pso_compilation },
        { "deferred_root_signature_compilation", m_deferred_root_signature_compilation },
        { "cache_path", m_cache_path },
        { "combined_cache_name", m_combined_cache_name },
        { "maximal_combined_cache_size", m_max_combined_cache_size },
        { "upload_heap_capacity", m_upload_heap_capacity },
        { "enable_async_compute", m_enable_async_compute },
        { "enable_async_copy", m_enable_async_copy },
        { "max_frames_in_flight", m_max_frames_in_flight },
        { "max_non_blocking_upload_buffer_allocation_timeout", m_max_non_blocking_upload_buffer_allocation_timeout },
        
        { "resource_view_descriptors_per_page", m_descriptors_per_page[static_cast<size_t>(DescriptorHeapType::cbv_srv_uav)] },
        { "resource_view_descriptor_heap_page_count", m_descriptor_heap_page_count[static_cast<size_t>(DescriptorHeapType::cbv_srv_uav)] },
        { "sampler_descriptors_per_page", m_descriptors_per_page[static_cast<size_t>(DescriptorHeapType::sampler)] },
        { "sampler_descriptor_heap_page_count", m_descriptor_heap_page_count[static_cast<size_t>(DescriptorHeapType::sampler)] },
        { "render_target_view_descriptors_per_page", m_descriptors_per_page[static_cast<size_t>(DescriptorHeapType::rtv)] },
        { "render_target_view_descriptor_heap_page_count", m_descriptor_heap_page_count[static_cast<size_t>(DescriptorHeapType::rtv)] },
        { "depth_stencil_view_descriptors_per_page", m_descriptors_per_page[static_cast<size_t>(DescriptorHeapType::dsv)] },
        { "depth_stencil_view_descriptor_heap_page_count", m_descriptor_heap_page_count[static_cast<size_t>(DescriptorHeapType::dsv)] }

    };

    if (m_shader_lookup_directories.size())
        j["shader_lookup_directories"] = m_shader_lookup_directories;

    ofile << j;

    ofile.close();
}

uint8_t GlobalSettings::getNumberOfWorkers() const
{
    return m_number_of_workers;
}

bool GlobalSettings::isDeferredShaderCompilationOn() const
{
    return m_deferred_shader_compilation;
}

bool GlobalSettings::isDeferredPSOCompilationOn() const
{
    return m_deferred_pso_compilation;
}

bool GlobalSettings::isDeferredRootSignatureCompilationOn() const
{
    return m_deferred_root_signature_compilation;
}

std::vector<std::string> const& GlobalSettings::getShaderLookupDirectories() const
{
    return m_shader_lookup_directories;
}

std::string GlobalSettings::getCacheDirectory() const
{
    return m_cache_path;
}

std::string GlobalSettings::getCombinedCacheName() const
{
    return m_combined_cache_name;
}

uint64_t GlobalSettings::getMaxCombinedCacheSize() const
{
    return m_max_combined_cache_size;
}

uint32_t GlobalSettings::getDescriptorHeapPageCapacity(DescriptorHeapType descriptor_heap_type) const
{
    return m_descriptors_per_page[static_cast<size_t>(descriptor_heap_type)];
}

uint32_t GlobalSettings::getDescriptorHeapPageCount(DescriptorHeapType descriptor_heap_type) const
{
    return m_descriptor_heap_page_count[static_cast<size_t>(descriptor_heap_type)];
}

uint32_t GlobalSettings::getUploadHeapCapacity() const
{
    return m_upload_heap_capacity;
}

bool GlobalSettings::isAsyncComputeEnabled() const
{
    return m_enable_async_compute;
}

bool GlobalSettings::isAsyncCopyEnabled() const
{
    return m_enable_async_copy;
}

uint16_t GlobalSettings::getMaxFramesInFlight() const
{
    return m_max_frames_in_flight;
}

uint32_t GlobalSettings::getMaxNonBlockingUploadBufferAllocationTimeout() const
{
    return m_max_non_blocking_upload_buffer_allocation_timeout;
}

void GlobalSettings::setNumberOfWorkers(uint8_t num_workers)
{
    m_number_of_workers = num_workers;
}

void GlobalSettings::setIsDeferredShaderCompilationOn(bool is_enabled)
{
    if (is_enabled && !m_deferred_pso_compilation)
    {
        misc::Log::retrieve()->out("WARNING: cannot enable deferred shader compilation while "
            "deferred PSO compilation is disabled", misc::LogMessageType::exclamation);
        m_deferred_shader_compilation = false;
    }
    else 
        m_deferred_shader_compilation = is_enabled;
}

void GlobalSettings::setIsDeferredPSOCompilationOn(bool is_enabled)
{
    m_deferred_pso_compilation = is_enabled;
}

void GlobalSettings::setIsDeferredRootSignatureCompilationOn(bool is_enabled)
{
    if (is_enabled && !m_deferred_pso_compilation)
    {
        misc::Log::retrieve()->out("WARNING: cannot enable deferred root signature compilation while "
            "deferred PSO compilation is disabled", misc::LogMessageType::exclamation);
        m_deferred_root_signature_compilation = false;
    }
    m_deferred_root_signature_compilation = is_enabled;
}

void GlobalSettings::addShaderLookupDirectory(std::string const & path)
{
    m_shader_lookup_directories.push_back(path);
}

void GlobalSettings::clearShaderLookupDirectories()
{
    m_shader_lookup_directories.clear();
}

void GlobalSettings::setCacheDirectory(std::string const& path)
{
    m_cache_path = path;
}

void GlobalSettings::setCacheName(std::string const& name)
{
    m_combined_cache_name = name;
}

void GlobalSettings::setIsAsyncComputeEnabled(bool is_enabled)
{
    m_enable_async_compute = is_enabled;
}

void GlobalSettings::setIsAsyncCopyEnabled(bool is_enabled)
{
    m_enable_async_copy = is_enabled;
}
