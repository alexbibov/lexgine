#ifndef LEXGINE_CORE_MISC_DATA_BLOB

#include <d3d12.h>
#include <wrl.h>

namespace lexgine {namespace core {

//! Interface for an arbitrary data blob, which may originate from different sources
//! This interface is completely OS-agnostic
class DataBlob
{
public:

    void* data() const;    //! returns pointer to data contained in the blob
    size_t size() const;    //! returns size of the blob
    bool isNull() const;    //! returns 'true' if this object contains no data blob

    operator bool() const;    //! returns 'true' if this object contains data blob (equivalent to !isNull())

    DataBlob();
    DataBlob(DataBlob const& other) = default;
    DataBlob(DataBlob&& other) = default;
    DataBlob(void *p_blob_data, size_t blob_size);

    DataBlob& operator=(DataBlob const&) = default;
    DataBlob& operator=(DataBlob&& other) = default;

    virtual ~DataBlob();

protected:
    void declareBufferPointer(void* ptr);

private:
    void* m_p_data;    //!< pointer to the data contained in the blob
    size_t m_size;    //!< size of the blob
};


//! Implements Direct3D data blob. This interface is tailored for windows (i.e. not OS-agnostic)
class D3DDataBlob : public DataBlob
{
public:
    D3DDataBlob();
    D3DDataBlob(nullptr_t);
    D3DDataBlob(Microsoft::WRL::ComPtr<ID3DBlob> const& blob);
    D3DDataBlob(size_t blob_size);

    Microsoft::WRL::ComPtr<ID3DBlob> native() const;    //! returns native pointer to Direct3D blob interface

private:
    Microsoft::WRL::ComPtr<ID3DBlob> m_blob;    //!< encapsulated Direct3D blob interface
};


//! Implements simplified automated memory data chunk with unique ownership that automatically allocates
//! a memory buffer of given size on creation and deallocates it on destruction. This class is API- and OS- agnostic
class DataChunk : public DataBlob
{
public:
    DataChunk() = default;    //! empty dummy data chunk with no actual memory allocation
    DataChunk(DataChunk const&) = delete;    // object implements unique ownership; hence, copying is forbidden
    DataChunk(DataChunk&&) = default;
    DataChunk(nullptr_t);    //! empty data chunk without actual memory tied to it
    DataChunk(size_t chunk_size);    //! creates new data chunk of given size

    DataChunk& operator=(DataChunk const&) = delete;
    DataChunk& operator=(DataChunk&&) = default;

    ~DataChunk();    //! destroys the data chunk and deallocates the memory buffer associated with it
};

}}

#define LEXGINE_CORE_MISC_DATA_BLOB
#endif
