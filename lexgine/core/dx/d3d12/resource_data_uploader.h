#ifndef LEXGINE_CORE_DX_D3D12_RESOURCE_DATA_UPLOADER_H
#define LEXGINE_CORE_DX_D3D12_RESOURCE_DATA_UPLOADER_H

#include <list>

#include "resource.h"
#include "command_list.h"
#include "upload_buffer_allocator.h"

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/entity.h"
#include "lexgine/core/data_blob.h"


namespace lexgine::core::dx::d3d12 {

//! Helper: implements uploading of placed subresources to the GPU-side
class ResourceDataUploader : public NamedEntity<class_names::D3D12_ResourceDataUploader>
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
        ResourceState destination_resource_state;    //!< state, in which the destination resource is expected to reside at the moment when the data upload happens
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



    ResourceDataUploader(Globals& globals, uint64_t offset, uint64_t capacity);    //! attaches data uploader to an upload heap at the given offset with the given upload buffer size
    ResourceDataUploader(ResourceDataUploader const&) = delete;
    ResourceDataUploader(ResourceDataUploader&&) = delete;


    /*! adds new upload task to the list of scheduled tasks and puts the related source data into intermediate upload buffer. This essentially means that after invoking this
     function the buffer containing the original source data can be deallocated as the data have already bean transfered to the upload heap owned by the data uploader.
     This however does not mean that the data have actually been transfered to the destination resources as the actual transaction begins only after invocation of function upload().
    */
    void addResourceForUpload(DestinationDescriptor const& destination_descriptor, SourceDescriptor const& source_descriptor);

    /*! executes all previously schedules upload tasks. This function returns immediately
     without making sure that the data has actually finished uploading
    */
    void upload(); 

    void waitUntilUploadIsFinished() const;    //! blocks calling thread until all upload tasks are finished
    bool isUploadFinished() const;    //! returns 'true' if all uploads have been finished and 'false' otherwise

private:
    Device& m_device;    //!< device object corresponding to the uploader
    DedicatedUploadDataStreamAllocator m_upload_buffer_allocator;    //!< upload buffer allocation manager
    CommandList m_upload_commands_list;    //!< command list intended to contain upload commands
    bool m_upload_command_list_needs_reset;
    ResourceState m_copy_destination_resource_state;
};

}

#endif
