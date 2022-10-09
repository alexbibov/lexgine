#include "dx_compiler_proxy.h"
#include "engine/core/misc/misc.h"
#include "engine/core/exception.h"
#include "engine/core/global_settings.h"

#include <vector>


using namespace lexgine::core;
using namespace lexgine::core::dx;
using namespace lexgine::core::dx::dxcompilation;


namespace
{

std::vector<wchar_t const*> convertParametersIntoCommandLineArgs(
    HLSLCompilationOptimizationLevel optimization_level,
    bool strict_mode, bool force_all_resources_be_bound,
    bool force_ieee_standard, bool treat_warnings_as_errors, bool enable_validation,
    bool enable_debug_inforamtion, bool enable_16bit_types)
{
    std::vector<wchar_t const*> rv{}; rv.reserve(10);
    switch (optimization_level)
    {
    case HLSLCompilationOptimizationLevel::level_no:
        rv.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);
        break;

    case HLSLCompilationOptimizationLevel::level0:
        rv.push_back(DXC_ARG_OPTIMIZATION_LEVEL0);
        break;

    case HLSLCompilationOptimizationLevel::level1:
        rv.push_back(DXC_ARG_OPTIMIZATION_LEVEL1);
        break;

    case HLSLCompilationOptimizationLevel::level2:
        rv.push_back(DXC_ARG_OPTIMIZATION_LEVEL2);
        break;

    case HLSLCompilationOptimizationLevel::level3:
        rv.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
        break;
    }

    if (strict_mode) rv.push_back(DXC_ARG_ENABLE_STRICTNESS);
    if (force_all_resources_be_bound)
    {
        rv.push_back(DXC_ARG_ALL_RESOURCES_BOUND);
        rv.push_back(DXC_ARG_AVOID_FLOW_CONTROL);    // avoid flow control constructs in DXIL
    }
    if (force_ieee_standard) rv.push_back(DXC_ARG_IEEE_STRICTNESS);
    if (treat_warnings_as_errors) rv.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);
    if (!enable_validation) rv.push_back(DXC_ARG_SKIP_VALIDATION);
    if (enable_debug_inforamtion) rv.push_back(DXC_ARG_DEBUG);
    if (enable_16bit_types) rv.push_back(L"-enable-16bit-types");

    return rv;
}

}


DXCompilerProxy::DXCompilerProxy(GlobalSettings const& global_settings) :
    m_is_successfully_initialized{ false },
    m_dxc_result(global_settings.getNumberOfWorkers()),
    m_dxc_errors(global_settings.getNumberOfWorkers(), "unknown error"),
    m_last_comilation_attempt_source_name(global_settings.getNumberOfWorkers())
{
    DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler3), reinterpret_cast<LPVOID*>(m_dxc.GetAddressOf()));
    if (!m_dxc) {
        misc::Log::retrieve()->out("Unable to instantiate LLVM-based HLSL compiler. Shader models 6.0 and higher will not be available", misc::LogMessageType::exclamation);
        m_is_successfully_initialized = false;
    }

    DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcUtils), reinterpret_cast<LPVOID*>(m_dxc_utils.GetAddressOf()));
    if (!m_dxc_utils) {
        misc::Log::retrieve()->out("Unable to initialize IDxcUtils interface. Shader models 6.0 and higher will not be available", misc::LogMessageType::exclamation);
        m_is_successfully_initialized = false;
    }
}

bool DXCompilerProxy::compile(uint8_t worker_id,
    std::string const& hlsl_source_code,
    std::string const& source_name,
    std::string const& entry_point_name,
    std::string const& target_profile_name,
    std::list<HLSLMacroDefinition> const& macro_definitions,
    HLSLCompilationOptimizationLevel optimization_level,
    bool strict_mode, bool force_all_resources_be_bound,
    bool force_ieee_standard, bool treat_warnings_as_errors, bool enable_validation,
    bool enable_debug_inforamtion, bool enable_16bit_types)
{
    m_last_comilation_attempt_source_name[worker_id] = source_name;

    std::vector<wchar_t const*> args =
        convertParametersIntoCommandLineArgs(optimization_level, strict_mode, force_all_resources_be_bound,
            force_ieee_standard, treat_warnings_as_errors, enable_validation, enable_debug_inforamtion, enable_16bit_types);

    std::vector<std::pair<std::wstring, std::wstring>> hlsl_define_name_and_value_pairs{};
    std::vector<DxcDefine> dxc_defines{};
    if (macro_definitions.size())
    {
        hlsl_define_name_and_value_pairs.reserve(macro_definitions.size());
        dxc_defines.reserve(macro_definitions.size());

        for (auto& d : macro_definitions)
        {
            hlsl_define_name_and_value_pairs.push_back(std::make_pair(misc::asciiStringToWstring(d.name), misc::asciiStringToWstring(d.value)));
            dxc_defines.push_back(DxcDefine{
                hlsl_define_name_and_value_pairs.back().first.c_str(),
                hlsl_define_name_and_value_pairs.back().second.c_str()
                });
        }
    }


    Microsoft::WRL::ComPtr<IDxcCompilerArgs> dxc_compiler_args{};
    {
        // Build compiler arguments
        HRESULT hres = m_dxc_utils->BuildArguments(misc::asciiStringToWstring(source_name).c_str(),
            misc::asciiStringToWstring(entry_point_name).c_str(), misc::asciiStringToWstring(target_profile_name).c_str(), args.data(), static_cast<UINT32>(args.size()),
            dxc_defines.data(), static_cast<UINT32>(dxc_defines.size()), dxc_compiler_args.GetAddressOf());

        if (hres != S_OK && hres != S_FALSE)
        {
            misc::Log::retrieve()->out("Unable to build arguments for HLSL source \"" + source_name + "\". The source code compilation is unable to proceed", misc::LogMessageType::exclamation);
            return false;
        }
    }


    {
        // Compile HLSL code
        DxcBuffer hlsl_source_dxc_buffer{};
        hlsl_source_dxc_buffer.Ptr = hlsl_source_code.c_str();
        hlsl_source_dxc_buffer.Size = hlsl_source_code.length();


        HRESULT hres = m_dxc->Compile(
            &hlsl_source_dxc_buffer,
            dxc_compiler_args->GetArguments(),
            dxc_compiler_args->GetCount(),
            nullptr,
            __uuidof(IDxcResult),
            reinterpret_cast<LPVOID*>(m_dxc_result[worker_id].ReleaseAndGetAddressOf())
        );

        if (hres != S_OK && hres != S_FALSE) return false;


        if (m_dxc_result[worker_id]->HasOutput(DXC_OUT_KIND::DXC_OUT_ERRORS))
        {
            Microsoft::WRL::ComPtr<IDxcBlobUtf8> dxc_errors_blob{ nullptr };
            Microsoft::WRL::ComPtr<IDxcBlobUtf16> dxc_result_name{ nullptr };

            HRESULT hres = m_dxc_result[worker_id]->GetOutput(DXC_OUT_KIND::DXC_OUT_ERRORS,
                __uuidof(IDxcBlobUtf8), reinterpret_cast<void**>(dxc_errors_blob.GetAddressOf()), dxc_result_name.GetAddressOf());

            if (hres != S_OK && hres != S_FALSE)
            {
                misc::Log::retrieve()->out("Unable to retrieve status of HLSL source compilation \"" + source_name + "\". "
                    "The compilation may have produced incorrect output.", misc::LogMessageType::exclamation);
            }
            else
            {
                auto error_message = std::string{ static_cast<char const*>(dxc_errors_blob->GetStringPointer()) };
                if (!error_message.empty())
                {
                    m_dxc_errors[worker_id] = error_message;
                    return false;
                }
            }
        }
    }

    return true;
}

misc::Optional<D3DDataBlob> DXCompilerProxy::result(uint8_t worker_id) const
{
    if (m_dxc_result[worker_id] && m_dxc_result[worker_id]->HasOutput(DXC_OUT_OBJECT))
    {
        Microsoft::WRL::ComPtr<ID3DBlob> blob{ nullptr };
        Microsoft::WRL::ComPtr<IDxcBlobUtf16> dxc_result_name{ nullptr };
        HRESULT hres = m_dxc_result[worker_id]->GetOutput(DXC_OUT_OBJECT,
            __uuidof(ID3DBlob), reinterpret_cast<void**>(blob.GetAddressOf()), dxc_result_name.GetAddressOf());
        if (hres != S_OK && hres != S_FALSE) return misc::Optional<D3DDataBlob>{};

        return misc::Optional<D3DDataBlob>{blob};
    }

    return misc::Optional<D3DDataBlob>{};
}

std::string DXCompilerProxy::errors(uint8_t worker_id) const
{
    return m_dxc_errors[worker_id];
}