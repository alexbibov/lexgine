#ifndef LEXGINE_CORE_DX_DXCOMPILATION_DX_COMPILER_PROXY_H
#define LEXGINE_CORE_DX_DXCOMPILATION_DX_COMPILER_PROXY_H

#include <d3d12.h>
#include <dxcapi.use.h>

#include <wrl/client.h>

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/misc/log.h"
#include "lexgine/core/data_blob.h"
#include "lexgine/core/misc/optional.h"
#include "common.h"


namespace lexgine { namespace core { namespace dx { namespace dxcompilation {

//! Proxy class that handles compilation of HLSL sources using LLVM-based Microsoft shader compiler
//! The class is designed to work in multi-threaded environment and is supposed to be thread-safe 
//! for concurrent worker IDs
class DXCompilerProxy final
{
public:
    DXCompilerProxy(GlobalSettings const& global_settings);

    //! Compiles provided HLSL source code and returns 'true' on success and 'false' on failure
    bool compile(uint8_t worker_id, std::string const& hlsl_source_code, std::string const& source_name,
        std::string const& entry_point_name, std::string const& target_profile_name,
        std::list<HLSLMacroDefinition> const& macro_definitions = std::list<HLSLMacroDefinition>{},
        HLSLCompilationOptimizationLevel optimization_level = HLSLCompilationOptimizationLevel::level3,
        bool strict_mode = true, bool force_all_resources_be_bound = false,
        bool force_ieee_standard = true, bool treat_warnings_as_errors = true, bool enable_validation = true,
        bool enable_debug_inforamtion = false, bool enable_16bit_types = false);

    //! given worker id returns compiled DXIL shader blob if the last compilation attempt on provided worker was successful
    misc::Optional<D3DDataBlob> result(uint8_t worker_id) const;

    //! given worker id retrieves user readable description from outcome of the last compilation attempt
    std::string errors(uint8_t worker_id) const;

private:
    dxc::DxcDllSupport m_dxc_dll;
    Microsoft::WRL::ComPtr<IDxcCompiler2> m_dxc;
    Microsoft::WRL::ComPtr<IDxcLibrary> m_dxc_lib;
    bool m_is_successfully_initialized;
    
    std::vector<Microsoft::WRL::ComPtr<IDxcOperationResult>> m_dxc_op_result;
    std::vector<std::string> m_dxc_errors;
    std::vector<std::string> m_last_comilation_attempt_source_name;
};


}}}}

#endif
