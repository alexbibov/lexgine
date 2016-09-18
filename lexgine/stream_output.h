#ifndef LEXGINE_CORE_STREAM_OUTPUT

#include <cstdint>
#include <string>
#include <utility>
#include <list>

namespace lexgine {namespace core {

//! API- and OS- agnostic declaration of geometry shader stream output (apparently not supported for Metal)
class StreamOutputDeclarationEntry
{
public:
    //! Initializes stream output declaration entry and attaches provided string name and name index to it.
    //! @param stream is zero-based index of the stream.
    //! @param name is the string name getting attached to the stream output. It can be string semantics as in Direct3D or simply variable name as in OpenGL.
    //! Exact meaning depends on the underlying API. @param name_index is index coupled with the string name when more then one stream outputs are using the
    //! same string name, again specific meaning relies on the graphics API.
    //! Note that @param name can be nullptr, in which case the entry defines gap in output stream where no data will be written; the size of this gap is
    //! defined by @param component_count that otherwise should be not greater than 4.
    //! @param start_component is the start vertex component at which the output should begin writing. Valid values are from 0 to 3.
    //! @param component_count is number of vertex components, to which to perform the output beginning from @param start_component.
    //! Valid values are from 1 to 4 (until @param name is not nullptr).
    //! @param slot is zero-based index of output buffer bound to the pipeline. The valid range is from 0 to 3.
    StreamOutputDeclarationEntry(uint32_t stream, char const* name, uint32_t name_index, uint8_t start_component, uint8_t component_count, uint8_t slot);

    uint32_t stream() const;    //! index of the stream associated with this stream output declaration entry
    std::string const& name() const;    //! semantic name of the stream output declaration entry
    uint32_t nameIndex() const;    //! semantic name index. Exact meaning depends on the underlying graphics API

    //! returns the first component to begin writing out to and the number of components receiving the data packed into std::pair in this order
    std::pair<uint8_t, uint8_t> outputComponents() const;

    uint8_t slot() const;    //! slot, to which the output buffer is bound

private:
    uint32_t m_stream;    //!< stream, to which the entry is associated
    std::string m_name;    //!< semantic name of the entry
    uint32_t m_name_index;   //!< index associated with semantic name of the entry. The exact usage depends on the underlying graphics API
    uint8_t m_start_component;    //!< the component, from which to begin writing out the data
    uint8_t m_component_count;    //!< number of components to write to beginning from (and including) the start component
    uint8_t m_slot;    //!< slot, to which the output buffer is bound
};


//! API- and OS- agnostic stream output declaration
struct StreamOutput
{
    std::list<StreamOutputDeclarationEntry> so_declarations;    //!< declaration entries determining how stream output is organized
    std::list<uint32_t> buffer_strides;    //!< strides for the stream output buffers, each stride is the size of element for that buffer
    bool disable_rasterization;    //!< if 'true' the rasterization stage is disabled, if 'false' the stream0 from geometry shader stage gets rasterized

    StreamOutput() = default; //! default stream output declaration: no stream output

    StreamOutput(std::list<StreamOutputDeclarationEntry> const& so_declarations, std::list<uint32_t> const& buffer_strides, bool disable_rasterization = false);
};


}}

#define LEXGINE_CORE_STREAM_OUTPUT
#endif
