#include "data_blob.h"

#include <utility>

using namespace lexgine::core;

void* DataBlob::data() const
{
    return m_p_data;
}

size_t DataBlob::size() const
{
    return m_size;
}

bool DataBlob::isNull() const
{
    return !m_p_data;
}

DataBlob::operator bool() const
{
    return m_p_data != nullptr;
}

DataBlob::DataBlob():
    m_p_data{ nullptr },
    m_size{ 0U }
{
}

DataBlob::DataBlob(void *p_blob_data, size_t blob_size):
    m_p_data{ p_blob_data },
    m_size{ blob_size }
{

}

DataBlob::~DataBlob()
{

}


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

Microsoft::WRL::ComPtr<ID3DBlob> D3DDataBlob::native() const
{
    return m_blob;
}


DataChunk::DataChunk(nullptr_t)
{
}

DataChunk::DataChunk(size_t chunk_size) : 
    DataBlob{ malloc(chunk_size), chunk_size }
{
}

DataChunk::~DataChunk()
{
    if (this->data())
        free(this->data());
}
