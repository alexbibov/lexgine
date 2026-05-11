#ifndef LEXGINE_CORE_DX_D3D12_D3D_DATA_BLOB_H
#define LEXGINE_CORE_DX_D3D12_D3D_DATA_BLOB_H

#include <d3d12.h>
#include <wrl.h>

#include "engine/core/data_blob.h"

namespace lexgine::core::dx::d3d12 {

//! Implements Direct3D data blob. This interface is tailored for windows (i.e. not OS-agnostic)
class D3DDataBlob : public DataBlob
{
public:
    D3DDataBlob();
    D3DDataBlob(nullptr_t);
    D3DDataBlob(D3DDataBlob const&) = default;
    D3DDataBlob(D3DDataBlob&&) = default;

    D3DDataBlob(Microsoft::WRL::ComPtr<ID3DBlob> const& blob);
    D3DDataBlob(size_t blob_size);

    D3DDataBlob& operator=(D3DDataBlob const&) = default;
    D3DDataBlob& operator=(D3DDataBlob&&) = default;
    D3DDataBlob& operator=(nullptr_t);    //! releases the memory associated with the blob

    Microsoft::WRL::ComPtr<ID3DBlob> native() const;    //! returns native pointer to Direct3D blob interface

private:
    Microsoft::WRL::ComPtr<ID3DBlob> m_blob;    //!< encapsulated Direct3D blob interface
};

}

#endif
