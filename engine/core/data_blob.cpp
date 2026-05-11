#include "data_blob.h"

#include <cassert>
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

DataBlob::DataBlob(nullptr_t): DataBlob{}
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

void DataBlob::declareBufferPointer(void* ptr)
{
    assert(!m_p_data);
    m_p_data = ptr;
}

DataBlob& DataBlob::operator=(nullptr_t)
{
    m_p_data = nullptr;
    m_size = 0U;
    return *this;
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

DataChunk& DataChunk::operator=(nullptr_t)
{
    if (this->data())
        free(this->data());
    DataBlob::operator=(nullptr);
    return *this;
}

SharedDataChunk::SharedDataChunk(nullptr_t)
{
}

SharedDataChunk::SharedDataChunk(size_t chunk_size):
    DataBlob{ nullptr, chunk_size },
    m_allocation_ptr{ malloc(chunk_size), free }
{
    declareBufferPointer(m_allocation_ptr.get());
}

SharedDataChunk& SharedDataChunk::operator=(nullptr_t)
{
    m_allocation_ptr.reset();
    DataBlob::operator=(nullptr);
    return *this;
}
