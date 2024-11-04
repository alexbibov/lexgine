#ifndef LEXGINE_CORE_DX_D3D12_RESOURCE_DATA_UPLOADER_H
#define LEXGINE_CORE_DX_D3D12_RESOURCE_DATA_UPLOADER_H

#include <list>

#include "resource.h"
#include "command_list.h"
#include "upload_buffer_allocator.h"

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/entity.h"
#include "engine/core/data_blob.h"
#include "engine/core/data_blob.h"


namespace lexgine::core::dx::d3d12 {

//! Helper: implements uploading of placed subresources to the GPU-side
class ResourceDataUploader : public NamedEntity<class_names::D3D12_ResourceDataUploader>
{
public:
    //! Structure that describes destination of the data being uploaded
    struct DestinationDescriptor
    {
        //! Subresource segment of the destination resource that will get the data (valid only for textures)
        struct SubresourceSegment
        {
            uint32_t first_subresource;    //!< first sub-resource of the destination resource, to which the data being uploaded will get written
            uint32_t num_subresources;    //!< number of sub-resources that will receive the data
        };

        //! Memory segment of the destination resource that will receive the data
        union DestinationSegment
        {
            SubresourceSegment subresources;    //!< sub-resources of the destination resource that will get the source data (valid only for texture resources)
            uint64_t base_offset;    //!< base offset, at which to start writing the data in the destination resource (valid only for buffer resources)
        };

        Resource const* p_destination_resource;    //!< destination resource receiving the data being uploaded
        ResourceState destination_resource_state;    //!< state, in which the destination resource is expected to reside at the moment when the data upload happens
        DestinationSegment segment;    //!< segment of the destination resource that will receive the source data
    };


    //! Describes texture source dataset
    struct TextureSourceDescriptor
    {
        // Describes single subresource from the source data 
        struct Subresource
        {
            void const* p_data;    //!< pointer to the source subresource data
            size_t row_pitch;    //!< size in bytes of a single row in the subresource
            size_t slice_pitch;    //!< size in bytes of a single data slice in the subresource
        };

        std::vector<Subresource> subresources;
    };

    //! Describes buffer source dataset
    struct BufferSourceDescriptor
    {
        void const* p_data;    //!< buffer source data
        size_t buffer_size;    //!< size of the buffer given in bytes
    };


    ResourceDataUploader(Globals& globals, DedicatedUploadDataStreamAllocator& upload_buffer_allocator);
    ResourceDataUploader(ResourceDataUploader const&) = delete;
    // ResourceDataUploader(ResourceDataUploader&&) = delete;

    //! Schedules new texture resource for upload
    bool addResourceForUpload(DestinationDescriptor const& destination_descriptor,
        TextureSourceDescriptor const& source_descriptor);

    //! Schedules new buffer resource for upload
    bool addResourceForUpload(DestinationDescriptor const& destination_descriptor,
        BufferSourceDescriptor const& source_descriptor);


    /*! executes all previously schedules upload tasks. This function returns immediately
     without making sure that the data has actually finished uploading
    */
    void upload();

    Resource const& sourceBuffer() const;    //! returns source buffer used for data uploading

    void waitUntilUploadIsFinished() const;    //! blocks calling thread until all upload tasks are finished
    bool isUploadFinished() const;    //! returns 'true' if all uploads have been finished and 'false' otherwise

private:
    void beginCopy(DestinationDescriptor const& destination_descriptor);
    void endCopy(DestinationDescriptor const& destination_descriptor);

private:
    Device& m_device;    //!< device object corresponding to the uploader
    bool m_is_async_copy_enabled;
    DedicatedUploadDataStreamAllocator& m_upload_buffer_allocator;    //!< upload buffer allocation manager
    CommandList m_upload_command_list;    //!< command list intended to contain upload commands
    bool m_upload_command_list_needs_reset{ true };
};

}

#endif
