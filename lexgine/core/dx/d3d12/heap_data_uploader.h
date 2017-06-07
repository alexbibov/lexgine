#ifndef LEXGINE_CORE_DX_D3D12_HEAP_DATA_UPLOADER_H

#include <list>

#include "resource.h"
#include "../../entity.h"
#include "command_list.h"


namespace lexgine {namespace core {namespace dx {namespace d3d12 {

//! Helper: implements uploading of placed subresources to the GPU-side
class HeapDataUploader : public NamedEntity<class_names::D3D12HeapDataUploader>
{
public:
    //! Structure that describes destination of the data being uploaded
    struct DestinationDescriptor
    {
        //! Subresource segment of the destination resource that will get the data (valid only for textures)
        struct DestinationDescriptorSubresourceSegment
        {
            uint32_t first_subresource;    //!< first sub-resource of the destination resource, to which the data being uploaded will get written
            uint32_t num_subresources;    //!< number of sub-resources that will receive the data
        };

        //! Memory segment of the destination resource that will receive the data
        union DestinationDescriptorSegment
        {
            DestinationDescriptorSubresourceSegment subresources;    //!< sub-resources of the destination resource that will get the source data (valid only for texture resources)
            uint64_t base_offset;    //!< base offset, at which to start writing the data in the destination resource (valid only for buffer resources)
        };

        //! Describes type of the memory segment of the destination resource
        enum DestinationDescriptorSegmentType { texture, buffer };

        Resource const* p_destination_resource;    //!< destination resource receiving the data being uploaded
        DestinationDescriptorSegment segment;    //!< segment of the destination resource that will receive the source data
        DestinationDescriptorSegmentType segment_type;    //!< type of memory segment of the target resource
    };


    //! Structure that describes source data that will be uploaded to the destination resource
    struct SourceDescriptor
    {
        void* p_source_data;    //!< pointer to the source data buffer to be uploaded
        uint64_t row_pitch;    //!< row pitch of the source data. In case of buffer resources this member contains the size of the data block to upload.
        uint64_t slice_pitch;    //!< slice pitch of the source data. Not used for buffer resources.
    };



    HeapDataUploader(Heap& upload_heap, uint64_t offset, uint64_t size);    //! attaches data uploader to an upload heap at the given offset with the given upload buffer size
    HeapDataUploader(HeapDataUploader const&) = delete;
    HeapDataUploader(HeapDataUploader&&) = delete;


    /*! adds new upload task to the list of scheduled tasks and puts the related source data into intermediate upload buffer. This essentially means that after invoking this
     function the buffer containing the original source data can be deallocated as the data have already bean transfered to the upload heap owned by the data uploader.
     This however does not mean that the data have actually been transfered to the destination resources as the actual transaction happens only during invocation of function upload().
    */
    void addResourceForUpload(DestinationDescriptor const& destination_descriptor, SourceDescriptor const& source_descriptor);


    /*! executes all previously scheduled upload tasks. This function performs actual data transfer from the upload buffer to the target GPU memory heaps. Note that the destination
     resources must be alive by the time of invocation of this function. The original source data buffers are not required to be available when this function is called since their contents
     have already been moved to intermediate upload buffer during registration of the related upload task. The command list referenced in the input to this function identifies, which hardware
     command list will get responsibility to transfer the data from the CPU-side to the GPU-side
    */
    void upload(CommandList& upload_worker_list);


    uint64_t getTransactionSize() const;    //! returns the total size of the upload transaction (sum of sizes of all individual upload tasks)


private:
    //! describes single upload task for the uploader
    struct upload_task
    {
        SourceDescriptor source_descriptor;
        DestinationDescriptor destination_descriptor;
        DataChunk subresource_footprints_buffer;
    };


    Heap& m_heap;    //!< upload heap used by the data uploader
    uint64_t m_offset;    //!< offset in the upload heap, at which the data uploader is registered
    uint64_t m_size;    //!< size of upload buffer used by the data uploader
    uint64_t m_transaction_size;    //!< size of all upload tasks assigned to the uploader

    Resource m_upload_buffer;    //!< native reference to the upload buffer

    std::list<upload_task> m_upload_tasks;    //!< list of upload tasks
};

}}}}

#define LEXGINE_CORE_DX_D3D12_HEAP_DATA_UPLOADER_H
#endif
