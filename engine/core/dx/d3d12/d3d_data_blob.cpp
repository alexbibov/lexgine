#include "d3d_data_blob.h"
#include "engine/core/exception.h"

#include <d3dcompiler.h>

using namespace lexgine::core::dx::d3d12;


D3DDataBlob::D3DDataBlob():
    m_blob{ nullptr }
{
}

D3DDataBlob::D3DDataBlob(nullptr_t) :
    m_blob{ nullptr }
{
}

D3DDataBlob::D3DDataBlob(Microsoft::WRL::ComPtr<ID3DBlob> const& blob) :
    DataBlob{ blob->GetBufferPointer(), blob->GetBufferSize() },
    m_blob{ blob }
{

}

D3DDataBlob::D3DDataBlob(size_t blob_size):
    DataBlob{ nullptr, blob_size },
    m_blob{ nullptr }
{
    HRESULT res = D3DCreateBlob(static_cast<SIZE_T>(blob_size), m_blob.GetAddressOf());
    if (res != S_OK)
    {
        LEXGINE_THROW_ERROR("Unable to create Direct3D data blob");
    }
    declareBufferPointer(m_blob->GetBufferPointer());
}

Microsoft::WRL::ComPtr<ID3DBlob> D3DDataBlob::native() const
{
    return m_blob;
}

D3DDataBlob& D3DDataBlob::operator=(nullptr_t)
{
    m_blob = nullptr;
    DataBlob::operator=(nullptr);
    return *this;
}
