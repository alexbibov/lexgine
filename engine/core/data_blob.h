#ifndef LEXGINE_CORE_DATA_BLOB

#include <memory>

namespace lexgine::core {

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
    DataBlob(nullptr_t);
    DataBlob(DataBlob const& other) = default;
    DataBlob(DataBlob&& other) = default;

    /*! creates data blob around provided pointer. Note that the ownership of the pointer data is not transfered
     to the blob, so it is the caller's responsibility to perform required memory clean-ups when necessary
    */
    DataBlob(void *p_blob_data, size_t blob_size);     

    DataBlob& operator=(DataBlob const&) = default;
    DataBlob& operator=(DataBlob&& other) = default;
    DataBlob& operator=(nullptr_t);    //! releases the memory associated with the blob

    virtual ~DataBlob();

protected:
    void declareBufferPointer(void* ptr);

private:
    void* m_p_data;    //!< pointer to the data contained in the blob
    size_t m_size;    //!< size of the blob
};


/*! Implements simplified automated memory data chunk with unique ownership that automatically allocates
 a memory buffer of given size on creation and deallocates it on destruction. This class is API- and OS- agnostic
*/
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
    DataChunk& operator=(nullptr_t);    //! releases the memory associated with the chunk

    ~DataChunk();    //! destroys the data chunk and deallocates the memory buffer associated with it
};

/*! Implements memory chunk with shared ownership based on reference counting. The memory is released as soon as the counter reaches
 the value of zero. Thread-safe, but take into account that the underlying shared pointer, which implements reference counting
 is based on atomics, which may introduce additional overhead.
*/
class SharedDataChunk : public DataBlob
{
public:
    SharedDataChunk() = default;    //! empty memory block without actual memory allocation
    SharedDataChunk(SharedDataChunk const&) = default;
    SharedDataChunk(SharedDataChunk&&) = default;
    SharedDataChunk(nullptr_t);    //! empty data chunk without actual memory
    SharedDataChunk(size_t chunk_size);    //! creates new data chunk with requested size of memory allocation associated to it

    SharedDataChunk& operator=(SharedDataChunk const&) = default;
    SharedDataChunk& operator=(SharedDataChunk&&) = default;
    SharedDataChunk& operator=(nullptr_t);    //! releases the memory associated with the chunk

    ~SharedDataChunk() = default;

private:
    std::shared_ptr<void> m_allocation_ptr;
};

}

#define LEXGINE_CORE_DATA_BLOB
#endif
