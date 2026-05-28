#ifndef LEXGINE_CORE_DX_D3D12_CACHES_HLSL_SHADER_BLOB_CACHE_H
#define LEXGINE_CORE_DX_D3D12_CACHES_HLSL_SHADER_BLOB_CACHE_H

#include <atomic>
#include <filesystem>
#include <functional>
#include <future>
#include <list>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "engine/core/class_names.h"
#include "engine/core/dx/d3d12/d3d_data_blob.h"
#include "engine/core/dx/dxcompilation/common.h"
#include "engine/core/entity.h"
#include "engine/core/global_settings.h"
#include "engine/core/globals.h"
#include "engine/core/gpu_data_blob_cache_key.h"
#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/misc/datetime.h"
#include "engine/core/shader_source_code_preprocessor.h"

namespace lexgine::core::dx::d3d12::caches {

class HLSLTranslationUnit
{
public:
    std::string const& name() const { return m_source_name; }
    std::string const& source() const { return m_hlsl_source_code; }
    misc::DateTime const& timestamp() const { return m_timestamp; }

    core::Globals& globals() const { return m_globals; }
    virtual misc::HashValue const* hash() const;

protected:
    HLSLTranslationUnit(Globals& globals)
        : m_globals { globals }
    {
    }

protected:
    core::Globals& m_globals;
    std::string m_source_name;
    std::string m_hlsl_source_code;
    misc::DateTime m_timestamp;
    mutable std::unique_ptr<misc::HashValue> m_hash_value;
};

class HLSLFileTranslationUnit final : public HLSLTranslationUnit
{
public:
    HLSLFileTranslationUnit(Globals& globals, std::string const& source_name, std::filesystem::path const& file_path);
    std::filesystem::path const& pathToShader() const { return m_path_to_shader; }
    misc::HashValue const* hash() const override;

private:
    std::filesystem::path m_path_to_shader;
};

class HLSLSourceTranslationUnit final : public HLSLTranslationUnit
{
public:
    HLSLSourceTranslationUnit(Globals& globals, std::string const& source_name, std::string const& hlsl_source_code)
        : HLSLTranslationUnit { globals }
    {
        m_source_name = source_name;
        auto const* global_settings = globals.get<lexgine::core::GlobalSettings>();
        m_hlsl_source_code = ShaderSourceCodePreprocessor {
            hlsl_source_code,
            ShaderSourceCodePreprocessor::SourceType::string,
            global_settings->getShaderLookupDirectories()
        }.getPreprocessedSource();
        m_timestamp = misc::DateTime::buildTime();
    }
};

enum class HLSLShaderBlobCompilationStatus
{
    NotScheduled,
    NotStarted,
    Started,
    Completed,
    Failed
};

struct HLSLShaderHandle
{
    GpuDataBlobCacheKey const* p_internal;
    bool operator==(HLSLShaderHandle const& other) const { return p_internal == other.p_internal; }
};

class HLSLShaderBlobCache final : public NamedEntity<class_names::D3D12_HLSLShaderBlobCache>
{
    friend class core::GpuDataBlobCacheKey;

public:
    HLSLShaderBlobCache(Globals& globals);
    ~HLSLShaderBlobCache();

    HLSLShaderHandle createHLSLShaderBlobCompilationContract(HLSLFileTranslationUnit const& hlsl_translation_unit,
        dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type, std::string const& shader_entry_point,
        std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions = std::list<dxcompilation::HLSLMacroDefinition> {},
        dxcompilation::HLSLCompilationOptimizationLevel optimization_level = dxcompilation::HLSLCompilationOptimizationLevel::level3,
        bool strict_mode = true, bool force_all_resources_be_bound = false,
        bool force_ieee_standard = true, bool treat_warnings_as_errors = true, bool enable_validation = true,
        bool enable_debug_information = false, bool enable_16bit_types = false);

    HLSLShaderHandle createHLSLShaderBlobCompilationContract(HLSLSourceTranslationUnit const& hlsl_translation_unit,
        dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type, std::string const& shader_entry_point,
        std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions = std::list<dxcompilation::HLSLMacroDefinition> {},
        dxcompilation::HLSLCompilationOptimizationLevel optimization_level = dxcompilation::HLSLCompilationOptimizationLevel::level3,
        bool strict_mode = true, bool force_all_resources_be_bound = false,
        bool force_ieee_standard = true, bool treat_warnings_as_errors = true, bool enable_validation = true,
        bool enable_debug_information = false, bool enable_16bit_types = false);

    void createShaderBlobs();
    void waitTillReady();

    std::pair<D3DDataBlob, HLSLShaderBlobCompilationStatus> getShaderBlob(HLSLShaderHandle handle) const;
    dxcompilation::ShaderType getShaderType(HLSLShaderHandle handle) const;
    dxcompilation::ShaderModel getShaderModel(HLSLShaderHandle handle) const;
    std::string getShaderCacheName(HLSLShaderHandle handle) const;
    std::string getCompilationLog(HLSLShaderHandle handle) const;
    bool isPrecached(HLSLShaderHandle handle) const;

    static std::pair<uint8_t, uint8_t> unpackShaderModelVersion(dxcompilation::ShaderModel shader_model);
    static std::string shaderModelAndTypeToTargetName(dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type);

private:
    struct ShaderContract
    {
        misc::DateTime timestamp;
        std::string hlsl_source;
        std::string source_name;
        dxcompilation::ShaderModel shader_model;
        dxcompilation::ShaderType shader_type;
        std::string shader_entry_point;
        std::list<dxcompilation::HLSLMacroDefinition> macro_definitions;
        dxcompilation::HLSLCompilationOptimizationLevel optimization_level;
        bool strict_mode;
        bool force_all_resources_be_bound;
        bool force_ieee_standard;
        bool treat_warnings_as_errors;
        bool enable_validation;
        bool enable_debug_information;
        bool enable_16bit_types;
        std::packaged_task<D3DDataBlob(HLSLShaderHandle, uint8_t)> task;
        std::atomic<HLSLShaderBlobCompilationStatus> status;
        bool precached = false;
        std::string compilation_log;

        ShaderContract(
            misc::DateTime const& timestamp,
            std::string const& hlsl_source,
            std::string const& source_name,
            dxcompilation::ShaderModel shader_model,
            dxcompilation::ShaderType shader_type,
            std::string const& shader_entry_point,
            std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions,
            dxcompilation::HLSLCompilationOptimizationLevel optimization_level,
            bool strict_mode,
            bool force_all_resources_be_bound,
            bool force_ieee_standard,
            bool treat_warnings_as_errors,
            bool enable_validation,
            bool enable_debug_information,
            bool enable_16bit_types,
            std::function<D3DDataBlob(HLSLShaderHandle, uint8_t)> const& op);
    };

    struct ShaderResult
    {
        ShaderContract* p_contract;
        std::future<D3DDataBlob> future;
        D3DDataBlob shader_blob;
    };

private:
    HLSLShaderHandle createHLSLShaderBlobCompilationContract(
        HLSLTranslationUnit const& hlsl_translation_unit,
        dxcompilation::ShaderModel shader_model,
        dxcompilation::ShaderType shader_type,
        std::string const& shader_entry_point,
        std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions,
        dxcompilation::HLSLCompilationOptimizationLevel optimization_level,
        bool strict_mode,
        bool force_all_resources_be_bound,
        bool force_ieee_standard,
        bool treat_warnings_as_errors,
        bool enable_validation,
        bool enable_debug_information,
        bool enable_16bit_types);

    D3DDataBlob compileShaderBlob(HLSLShaderHandle handle, uint8_t worker_id);

    GpuDataBlobCacheKey createGpuDataBlobCacheKey(
        HLSLTranslationUnit const& hlsl_translation_unit,
        dxcompilation::ShaderModel shader_model,
        dxcompilation::ShaderType shader_type,
        std::string const& shader_entry_point,
        std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions,
        dxcompilation::HLSLCompilationOptimizationLevel optimization_level,
        bool strict_mode,
        bool force_all_resources_be_bound,
        bool force_ieee_standard,
        bool treat_warnings_as_errors,
        bool enable_validation,
        bool enable_debug_information,
        bool enable_16bit_types
    ) const;

private:
    Globals& m_globals;
    GpuDataBlobCache& m_gpu_blob_cache;
    bool m_async_shader_compilation;
    mutable std::recursive_mutex m_lock;

    std::unordered_map<
        GpuDataBlobCacheKey,
        ShaderContract,
        GpuDataBlobCacheKeyHasher
    > m_contracts;

    std::vector<std::pair<HLSLShaderHandle, ShaderContract*>> m_unresolved_contracts;

    mutable std::unordered_map<
        HLSLShaderHandle,
        ShaderResult,
        LightWeightKeyHasher<HLSLShaderHandle>
    > m_cached_shaders;

    std::vector<std::thread> m_worker_threads;
};

}

#endif
