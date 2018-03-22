#ifndef LEXGINE_CORE_DX_DXCOMPILATION_DXCOMPILER_H
#define LEXGINE_CORE_DX_DXCOMPILATION_DXCOMPILER_H

#include <d3d12.h>
#include <dxcapi.use.h>

namespace lexgine { namespace core { namespace dx { namespace dxcompilation {

class DXCompiler final
{
public:
    DXCompiler();

private:
    dxc::DxcDllSupport m_dxc_dll;
};

}}}}

#endif
