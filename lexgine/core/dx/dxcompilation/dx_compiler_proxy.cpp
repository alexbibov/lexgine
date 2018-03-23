#include "dx_compiler_proxy.h"
#include "../../misc/misc.h"
#include "../../exception.h"

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
        rv.push_back(L"-Od");
        break;

    case HLSLCompilationOptimizationLevel::level0:
        rv.push_back(L"-O0");
        break;

    case HLSLCompilationOptimizationLevel::level1:
        rv.push_back(L"-O1");
        break;

    case HLSLCompilationOptimizationLevel::level2:
        rv.push_back(L"-O2");
        break;

    case HLSLCompilationOptimizationLevel::level3:
        rv.push_back(L"-O3");
        break;

    case HLSLCompilationOptimizationLevel::level4:
        rv.push_back(L"-O4");
        break;
    }

    if (strict_mode) rv.push_back(L"-Ges");
    if (force_all_resources_be_bound) rv.push_back(L"-all_resources_bound");
    if (force_ieee_standard) rv.push_back(L"-Gis");
    if (treat_warnings_as_errors) rv.push_back(L"-WX");
    if (!enable_validation) rv.push_back(L"-Vd");
    if (enable_debug_inforamtion) rv.push_back(L"-Zi");
    if (enable_16bit_types) rv.push_back(L"-enable-16bit-types");

    rv.push_back(L"-Gfa");    // avoid flow control constructs in DXIL
    rv.push_back(L"-HV 2018");    // force HLSL version 2018 for consistency

    return rv;
}

}


DXCompilerProxy::DXCompilerProxy():
    m_is_successfully_initialized{ true },
    m_dxc_op_result{ nullptr }
{
    m_dxc_dll.Initialize();

    m_dxc_dll.CreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler2), reinterpret_cast<IUnknown**>(m_dxc.GetAddressOf()));
    if (!m_dxc) {
        misc::Log::retrieve()->out("Unable to instantiate LLVM-based HLSL compiler. Shader models 6.0 and higher will not be available", misc::LogMessageType::exclamation);
        m_is_successfully_initialized = false;
    }

    m_dxc_dll.CreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), reinterpret_cast<IUnknown**>(m_dxc_lib.GetAddressOf()));
    if (!m_dxc_lib) {
        misc::Log::retrieve()->out("Unable to initialize IDxcLibrary interface. Shader models 6.0 and higher will not be available", misc::LogMessageType::exclamation);
        m_is_successfully_initialized = false;
    }
}

bool DXCompilerProxy::compile(std::string const& hlsl_source_code,
    std::string const& source_name, std::string const& entry_point_name, 
    std::string const& target_profile_name,
    std::list<HLSLMacroDefinition> const& macro_definitions,
    HLSLCompilationOptimizationLevel optimization_level, 
    bool strict_mode, bool force_all_resources_be_bound,
    bool force_ieee_standard, bool treat_warnings_as_errors, bool enable_validation,
    bool enable_debug_inforamtion, bool enable_16bit_types)
{
    m_last_compile_attempt_source_name = source_name;
    
    Microsoft::WRL::ComPtr<IDxcBlobEncoding> hlsl_source_dxc_blob{ nullptr };
    {
        // Convert source code into data blob with encoding

        DataChunk source_code_data_chunk{ hlsl_source_code.length() };
        memcpy(source_code_data_chunk.data(), hlsl_source_code.c_str(), hlsl_source_code.length());

        // Convert source code into blob
        HRESULT hres = m_dxc_lib->CreateBlobWithEncodingFromPinned(
            static_cast<LPBYTE>(source_code_data_chunk.data()),
            static_cast<UINT32>(source_code_data_chunk.size()),
            CP_UTF8, 
            hlsl_source_dxc_blob.GetAddressOf()
        );

        if (hres != S_OK && hres != S_FALSE)
            return false;
    }


    std::vector<wchar_t const*> args = convertParametersIntoCommandLineArgs(optimization_level, strict_mode, force_all_resources_be_bound,
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


    {
        // Compile HLSL code

        HRESULT hres = m_dxc->Compile(
            hlsl_source_dxc_blob.Get(),
            misc::asciiStringToWstring(source_name).c_str(),
            misc::asciiStringToWstring(entry_point_name).c_str(),
            misc::asciiStringToWstring(target_profile_name).c_str(),
            args.data(),
            static_cast<UINT32>(args.size()),
            dxc_defines.data(),
            static_cast<UINT32>(dxc_defines.size()),
            nullptr,
            m_dxc_op_result.ReleaseAndGetAddressOf()
        );

        if (hres != S_OK && hres != S_FALSE)
            return false;
    }

    return true;
}

misc::Optional<D3DDataBlob> DXCompilerProxy::result() const
{
    if (m_dxc_op_result)
    {
        Microsoft::WRL::ComPtr<ID3DBlob> blob{ nullptr };
        HRESULT hres = m_dxc_op_result->GetResult(reinterpret_cast<IDxcBlob**>(blob.GetAddressOf()));
        if (hres != S_OK && hres != S_FALSE) return misc::Optional<D3DDataBlob>{};

        return misc::Optional<D3DDataBlob>{blob};
    }

    return misc::Optional<D3DDataBlob>{};
}

std::string DXCompilerProxy::errors() const
{
    if (m_dxc_op_result)
    {
        Microsoft::WRL::ComPtr<IDxcBlobEncoding> error_blob_with_encoding{ nullptr };
        m_dxc_op_result->GetErrorBuffer(error_blob_with_encoding.GetAddressOf());

        if (!error_blob_with_encoding) return std::string{"unknown error"};

        return misc::wstringToAsciiString(static_cast<wchar_t*>(error_blob_with_encoding->GetBufferPointer()));
    }

    return std::string{};
}

DXCompilerProxy::operator bool() const
{
    return m_is_successfully_initialized && errors().length() == 0;
}
