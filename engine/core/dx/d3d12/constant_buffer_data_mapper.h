#ifndef LEXGINE_CORE_DX_D3D12_CONSTANT_BUFFER_DATA_MAPPER_H
#define LEXGINE_CORE_DX_D3D12_CONSTANT_BUFFER_DATA_MAPPER_H

#include <list>
#include "engine/core/misc/hashed_string.h"
#include "engine/core/misc/static_vector.h"
#include "engine/core/math/vector_types.h"
#include "engine/core/math/matrix_types.h"
#include "constant_buffer_reflection.h"

namespace lexgine::core::dx::d3d12 {

template<typename T>
struct ConstantElementProvider final
{
    static_assert(std::is_scalar<T>::value, "type is not supported");

    static void const* fetch(T const& element) { return &element; }
};

template<typename T, uint32_t nrows, uint32_t ncolumns, glm::qualifier Q>
struct ConstantElementProvider<math::matrix<T, nrows, ncolumns, Q>> final
{
    static void const* fetch(math::matrix<T, nrows, ncolumns, Q> const& element)
    { 
        return element.getRawData();
    }
};

template<typename T, uint32_t nelements, glm::qualifier Q>
struct ConstantElementProvider<math::vector<T, nelements, Q>> final
{
    static void const* fetch(math::vector<T, nelements, Q> const& element) { return element.getRawData(); }
};


class AbstractConstantDataProvider
{
public:
    virtual ~AbstractConstantDataProvider() = default;

    virtual void const* fetchDataElement(size_t element_index = 0U) const = 0;
    virtual size_t dataElementCount() const = 0;
};

template<typename T> class ConstantDataProvider : public AbstractConstantDataProvider
{
public:
    ConstantDataProvider(T const& value)
        : m_value{ value }
    {

    }

    void const* fetchDataElement(size_t element_index /* = 0U */) const override
    {
        return ConstantElementProvider<T>::fetch(m_value);
    }

    size_t dataElementCount() const override { return 1; }

private:
    T const& m_value;
};

template<typename T> class ConstantDataProvider<std::vector<T>> 
    : public AbstractConstantDataProvider
{
public:
    ConstantDataProvider(std::vector<T> const& value)
        : m_value{ value }
    {

    }

    void const* fetchDataElement(size_t element_index /* = 0U */) const override
    {
        T const& element = m_value[element_index];
        return ConstantElementProvider<T>::fetch(element);
    }

    size_t dataElementCount() const override { return m_value.size(); }

private:
    std::vector<T> const& m_value;
};


class ConstantBufferDataMapper final
{
public:
    explicit ConstantBufferDataMapper(ConstantBufferReflection const& reflection);

    template<typename T>
    void addDataBinding(std::string const& target_variable_name, T const& data_source)
    {
        m_writers.emplace_back(misc::HashedString{ target_variable_name },
            std::unique_ptr<AbstractConstantDataProvider>{new ConstantDataProvider<std::remove_const_t<std::remove_reference_t<T>>>{ data_source }});
    }

    void writeAllBoundData(uint64_t constant_buffer_allocation_base_address) const;

    size_t mappedDataSize() const { return m_reflection.size(); }

private:
    ConstantBufferReflection const& m_reflection;
    std::vector<std::pair<misc::HashedString, std::unique_ptr<AbstractConstantDataProvider>>> m_writers;
};

}

#endif