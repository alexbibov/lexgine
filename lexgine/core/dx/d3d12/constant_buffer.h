#ifndef LEXGINE_CORE_DX_D3D12_CONSTANT_BUFFER_H

#include "../../entity.h"
#include "../../class_names.h"
#include "d3d12_tools.h"
#include "../../data_blob.h"

#include <list>
#include <vector>
#include <memory>

namespace lexgine {namespace core {namespace dx {namespace d3d12 {

//! High-level API for constant buffer usage. This API is tailored for Direct3D 12
class ConstantBuffer final : public NamedEntity<class_names::D3D12ConstantBuffer>
{
public:
    template<typename T> class constant_buffer_entry;

private:
#include "constant_buffer_impl.inl"    // implementation details are inlined into separate header files in order to increase readability of the code that matters


public:
    template<typename T>
    class constant_buffer_entry
    {
    private:
        template<typename P> struct unconvert_helper;

    public:
        using value_type = T;

        //! returns currently set value of the constant
        T getValue() const
        {
            assert(m_constant_buffer.m_is_constructed);
            return unconvert_helper_type::getValue(*this);
        }

        //! assigns new value to a constant in the buffer
        void setValue(T const& value)
        {
            assert(m_constant_buffer.m_is_constructed);
            constant_buffer_hlsl_type_raw_data_converter<T>::convert(static_cast<char*>(m_constant_buffer.m_raw_data) + m_offset, value);
        }

        constant_buffer_entry(ConstantBuffer& constant_buffer, uint32_t offset, uint32_t size) :
            m_constant_buffer{ constant_buffer },
            m_offset{ offset },
            m_size{ size }
        {

        }

    private:
        template<typename P>
        struct unconvert_helper
        {
            static P const& getValue(constant_buffer_entry const& entry)
            {
                return constant_buffer_hlsl_type_raw_data_converter<P>::unconvert(static_cast<char*>(entry.m_constant_buffer.m_raw_data) + entry.m_offset);
            }
        };

        template<typename P>
        struct unconvert_helper<std::vector<P>>
        {
            static std::vector<P> getValue(constant_buffer_entry const& entry)
            {
                return constant_buffer_hlsl_type_raw_data_converter<std::vector<P>>::unconvert(static_cast<char*>(entry.m_constant_buffer.m_raw_data) + entry.m_offset, entry.m_size);
            }
        };

        using unconvert_helper_type = unconvert_helper<T>;

        ConstantBuffer& m_constant_buffer;    //!< reference to the constant buffer associated with the entry
        uint32_t m_offset;    //!< offset to the entry within the constant buffer
        uint32_t m_size;    //!< size that the entry occupies in the constant buffer
    };


public:

    ConstantBuffer();
    ConstantBuffer(ConstantBuffer const& other) = delete;
    ConstantBuffer(ConstantBuffer&& other);
    ~ConstantBuffer();

    //! Performs actual construction of the constant buffer
    void build();

    //! Returns size of the constant buffer represented in bytes
    size_t size() const;

    // Constant entries

    template<typename T>
    constant_buffer_entry<T> addScalar(std::string const& name, T constant)
    {
        DataChunk* p_data_chunk = new DataChunk{ sizeof(bool_to_uint32<T>::type) };
        constant_buffer_hlsl_type_raw_data_converter<T>::convert(p_data_chunk->data(), constant);
        return constant_buffer_entry{ *this, add_entry(name, p_data_chunk), p_data_chunk->size() };
    }

    template<typename T>
    constant_buffer_entry<std::vector<T>> addScalarArray(std::string const& name, std::vector<T> const& array_of_constants)
    {
        size_t const entry_size = sizeof(T)*array_of_constants.size();
        DataChunk* p_data_chunk = new DataChunk{ entry_size };
        constant_buffer_hlsl_type_raw_data_converter<T>::convert(p_data_chunk->data(), array_of_constants);
        return constant_buffer_entry{ *this, add_entry(name, p_data_chunk), p_data_chunk->size() };
    }


    // Vector entries

    template<typename T>
    constant_buffer_entry<math::tagVector2<T>> addVector(std::string const& name, math::tagVector2<T> const& vector2)
    {
        return add_vector(name, vector2);
    }

    template<typename T>
    constant_buffer_entry<math::tagVector3<T>> addVector(std::string const& name, math::tagVector3<T> const& vector3)
    {
        return add_vector(name, vector3);
    }

    template<typename T>
    constant_buffer_entry<math::tagVector4<T>> addVector(std::string const& name, math::tagVector4<T> const& vector4)
    {
        return add_vector(name, vector4);
    }

    template<typename T>
    constant_buffer_entry<std::vector<math::tagVector2<T>>> addVectorArray(std::string const& name, std::vector<math::tagVector2<T>> const& vector2_array)
    {
        return add_vector_array(name, vector2_array);
    }

    template<typename T>
    constant_buffer_entry<std::vector<math::tagVector3<T>>> addVectorArray(std::string const& name, std::vector<math::tagVector3<T>> const& vector3_array)
    {
        return add_vector_array(name, vector3_array);
    }

    template<typename T>
    constant_buffer_entry<std::vector<math::tagVector4<T>>> addVectorArray(std::string const& name, std::vector<math::tagVector4<T>> const& vector4_array)
    {
        return add_vector_array(name, vector4_array);
    }


    // Matrix entries

    template<typename T>
    constant_buffer_entry <math::shader_matrix_type<T, 2, 2>> addMatrix(std::string const& name, math::shader_matrix_type<T, 2, 2> const& hlsl_matrix_2x2)
    {
        return add_matrix(name, hlsl_matrix_2x2);
    }

    template<typename T>
    constant_buffer_entry <math::shader_matrix_type<T, 2, 3>> addMatrix(std::string const& name, math::shader_matrix_type<T, 2, 3> const& hlsl_matrix_2x3)
    {
        return add_matrix(name, hlsl_matrix_2x3);
    }

    template<typename T>
    constant_buffer_entry <math::shader_matrix_type<T, 2, 4>> addMatrix(std::string const& name, math::shader_matrix_type<T, 2, 4> const& hlsl_matrix_2x4)
    {
        return add_matrix(name, hlsl_matrix_2x4);
    }

    template<typename T>
    constant_buffer_entry <math::shader_matrix_type<T, 3, 2>> addMatrix(std::string const& name, math::shader_matrix_type<T, 3, 2> const& hlsl_matrix_3x2)
    {
        return add_matrix(name, hlsl_matrix_3x2);
    }

    template<typename T>
    constant_buffer_entry <math::shader_matrix_type<T, 3, 3>> addMatrix(std::string const& name, math::shader_matrix_type<T, 3, 3> const& hlsl_matrix_3x3)
    {
        return add_matrix(name, hlsl_matrix_3x3);
    }

    template<typename T>
    constant_buffer_entry <math::shader_matrix_type<T, 3, 4>> addMatrix(std::string const& name, math::shader_matrix_type<T, 3, 4> const& hlsl_matrix_3x4)
    {
        return add_matrix(name, hlsl_matrix_3x4);
    }

    template<typename T>
    constant_buffer_entry <math::shader_matrix_type<T, 4, 2>> addMatrix(std::string const& name, math::shader_matrix_type<T, 4, 2> const& hlsl_matrix_4x2)
    {
        return add_matrix(name, hlsl_matrix_4x2);
    }

    template<typename T>
    constant_buffer_entry <math::shader_matrix_type<T, 4, 3>> addMatrix(std::string const& name, math::shader_matrix_type<T, 4, 3> const& hlsl_matrix_4x3)
    {
        return add_matrix(name, hlsl_matrix_4x3);
    }

    template<typename T>
    constant_buffer_entry <math::shader_matrix_type<T, 4, 4>> addMatrix(std::string const& name, math::shader_matrix_type<T, 4, 4> const& hlsl_matrix_4x4)
    {
        return add_matrix(name, hlsl_matrix_4x4);
    }


    template<typename T>
    constant_buffer_entry<std::vector<math::shader_matrix_type<T, 2, 2>>> addMatrixArray(std::string const& name,
        std::vector<math::shader_matrix_type<T, 2, 2>> const& hlsl_matrix_2x2_array)
    {
        return add_matrix_array(name, hlsl_matrix_2x2_array);
    }

    template<typename T>
    constant_buffer_entry<std::vector<math::shader_matrix_type<T, 2, 3>>> addMatrixArray(std::string const& name,
        std::vector<math::shader_matrix_type<T, 2, 3>> const& hlsl_matrix_2x3_array)
    {
        return add_matrix_array(name, hlsl_matrix_2x3_array);
    }

    template<typename T>
    constant_buffer_entry<std::vector<math::shader_matrix_type<T, 2, 4>>> addMatrixArray(std::string const& name,
        std::vector<math::shader_matrix_type<T, 2, 4>> const& hlsl_matrix_2x4_array)
    {
        return add_matrix_array(name, hlsl_matrix_2x4_array);
    }

    template<typename T>
    constant_buffer_entry<std::vector<math::shader_matrix_type<T, 3, 2>>> addMatrixArray(std::string const& name,
        std::vector<math::shader_matrix_type<T, 3, 2>> const& hlsl_matrix_3x2_array)
    {
        return add_matrix_array(name, hlsl_matrix_3x2_array);
    }

    template<typename T>
    constant_buffer_entry<std::vector<math::shader_matrix_type<T, 3, 3>>> addMatrixArray(std::string const& name,
        std::vector<math::shader_matrix_type<T, 3, 3>> const& hlsl_matrix_3x3_array)
    {
        return add_matrix_array(name, hlsl_matrix_3x3_array);
    }

    template<typename T>
    constant_buffer_entry<std::vector<math::shader_matrix_type<T, 3, 4>>> addMatrixArray(std::string const& name,
        std::vector<math::shader_matrix_type<T, 3, 4>> const& hlsl_matrix_3x4_array)
    {
        return add_matrix_array(name, hlsl_matrix_3x4_array);
    }

    template<typename T>
    constant_buffer_entry<std::vector<math::shader_matrix_type<T, 4, 2>>> addMatrixArray(std::string const& name,
        std::vector<math::shader_matrix_type<T, 4, 2>> const& hlsl_matrix_4x2_array)
    {
        return add_matrix_array(name, hlsl_matrix_4x2_array);
    }

    template<typename T>
    constant_buffer_entry<std::vector<math::shader_matrix_type<T, 4, 3>>> addMatrixArray(std::string const& name,
        std::vector<math::shader_matrix_type<T, 4, 3>> const& hlsl_matrix_4x3_array)
    {
        return add_matrix_array(name, hlsl_matrix_4x3_array);
    }

    template<typename T>
    constant_buffer_entry<std::vector<math::shader_matrix_type<T, 4, 4>>> addMatrixArray(std::string const& name,
        std::vector<math::shader_matrix_type<T, 4, 4>> const& hlsl_matrix_4x4_array)
    {
        return add_matrix_array(name, hlsl_matrix_4x4_array);
    }


private:
    struct entry_desc
    {
        std::string name;    //!< semantic name of the constant
        uint32_t offset;    //!< offset to the data within the constant buffer
        std::unique_ptr<DataBlob> p_data;    //!< pointer to the data blob, which describes memory buffer currently containing the data
    };

    //! helper: registers new entry into the constant buffer and returns offset from the beginning of the buffer, at which the data from this entry will be stored
    uint32_t add_entry(std::string const& name, DataChunk* p_data_chunk, size_t element_size);

    std::list<entry_desc> m_constants;    //!< list of descriptors of the constants stored in the buffer
    uint32_t m_current_offset;    //!< current aligned offset in the constant buffer
    void* m_raw_data;    //!< pointer to the raw data buffer containing the actual data that will transfered onto the GPU side
    bool m_is_constructed;    //!< 'true' if the buffer has already been constructed
};

}}}}

#define LEXGINE_CORE_DX_D3D12_CONSTANT_BUFFER_H
#endif
