#ifndef LEXGINE_SCENEGRAPH_BUFFER_VIEW_H

#include <string>

#include "lexgine_scenegraph_fwd.h"

namespace lexgine::scenegraph
{

class BufferView
{
public:
    BufferView(const std::string& name, Buffer& target_buffer, size_t offset, size_t num_elements, size_t element_stride);



private:
    std::string m_name;
    Buffer& m_buffer;
    size_t m_offset;
    size_t m_num_elements;
    size_t m_element_stride;
};

}

#define LEXGINE_SCENEGRAPH_BUFFER_VIEW_H
#endif