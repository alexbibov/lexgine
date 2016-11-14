#ifndef LEXGINE_CORE_DX_D3D12_HEAP_DATA_UPLOADER_H

#include <list>

#include "resource.h"
#include "entity.h"

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
        uint64_t row_pitch;    //!< row pitch of the source data
        uint64_t slice_pitch;    //!< slice pitch of the source data
    };



    HeapDataUploader(Heap& upload_heap, uint64_t offset, uint64_t size);    //! attaches data uploader to an upload heap at the given offset with the given upload buffer size
    HeapDataUploader(HeapDataUploader const&) = delete;
    HeapDataUploader(HeapDataUploader&&) = delete;


    //! creates new upload task and adds it to the list of scheduled upload tasks
    void addResourceForUpload(DestinationDescriptor const& destination_descriptor, SourceDescriptor const& source_descriptor);


    /*! executes all previously scheduled upload tasks. This function causes actual copy of the source data into the destination resources. Note that all source data buffers and the
      corresponding destination resources should be alive by the time this function is invoked
    */
    void upload();

private:
    //! describes single upload task for the uploader
    struct upload_task
    {
        SourceDescriptor source_descriptor;
        DestinationDescriptor destination_descriptor;
        uint64_t task_size;
    };


    Heap& m_heap;    //!< upload heap used by the data uploader
    uint64_t m_offset;    //!< offset in the upload heap, at which the data uploader is registered
    uint64_t m_size;    //!< size of upload buffer used by the data uploader

    ComPtr<ID3D12Resource> m_upload_buffer;    //!< native reference to the upload buffer

    std::list<upload_task> m_upload_tasks;    //!< list of upload tasks
};

}}}}

#define LEXGINE_CORE_DX_D3D12_HEAP_DATA_UPLOADER_H
#endif
