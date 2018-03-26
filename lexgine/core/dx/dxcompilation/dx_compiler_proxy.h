#ifndef LEXGINE_CORE_DX_DXCOMPILATION_DX_COMPILER_PROXY_H
#define LEXGINE_CORE_DX_DXCOMPILATION_DX_COMPILER_PROXY_H

#include <d3d12.h>
#include <dxcapi.use.h>

#include <wrl/client.h>

#include "../../misc/log.h"
#include "../../data_blob.h"
#include "../../misc/optional.h"
#include "common.h"

namespace lexgine { namespace core { namespace dx { namespace dxcompilation {

// Proxy class that handles compilation of HLSL sources using LLVM-based Microsoft shader compiler
class DXCompilerProxy final
{
public:
    DXCompilerProxy();

    //! Compiles provided HLSL source code and returns 'true' on success and 'false' on failure
    bool compile(std::string const& hlsl_source_code, std::string const& source_name,
        std::string const& entry_point_name, std::string const& target_profile_name,
        std::list<HLSLMacroDefinition> const& macro_definitions = std::list<HLSLMacroDefinition>{},
        HLSLCompilationOptimizationLevel optimization_level = HLSLCompilationOptimizationLevel::level3,
        bool strict_mode = true, bool force_all_resources_be_bound = false,
        bool force_ieee_standard = true, bool treat_warnings_as_errors = true, bool enable_validation = true,
        bool enable_debug_inforamtion = false, bool enable_16bit_types = false);

    misc::Optional<D3DDataBlob> result() const;    //! returns compiled shader blob in case of success and invalid object in case of initialization or compilation failure
    std::string errors() const;    //! returns user-readable description of errors that were encountered during the last compilation attempt

    operator bool() const;    //! returns 'true' if the object is well-formed and 'false' in case of initialization or compilation error

private:
    dxc::DxcDllSupport m_dxc_dll;
    Microsoft::WRL::ComPtr<IDxcCompiler2> m_dxc;
    Microsoft::WRL::ComPtr<IDxcLibrary> m_dxc_lib;
    Microsoft::WRL::ComPtr<IDxcOperationResult> m_dxc_op_result;
    Microsoft::WRL::ComPtr<IDxcBlobEncoding> m_dxc_errors;
    bool m_is_successfully_initialized;
    std::string m_last_compile_attempt_source_name;
};

}}}}

#endif
