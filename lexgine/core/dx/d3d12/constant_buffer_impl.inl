//! helper template needed to substitute boolean types with uint32_t, which corresponds to representation of booleans in HLSL
template<typename T> struct bool_to_uint32 { using type = T; };
template<> struct bool_to_uint32<bool> { using type = uint32_t; };


#include "constant_buffer_impl_converters_scalar_types.inl"

#include "constant_buffer_impl_converters_vector_types.inl"

#include "constant_buffer_impl_converters_matrix_types.inl"


//! helper: adds non-boolean vector entry into the constant buffer
template<typename T>
constant_buffer_entry<T> add_vector(std::string const& name, T const& vector)
{
    size_t const entry_size = T::dimension * sizeof(bool_to_uint32<T::value_type>::type);
    DataChunk* p_data_chunk = new DataChunk{ entry_size };
    constant_buffer_hlsl_type_raw_data_converter<T>::convert(p_data_chunk->data(), vector);
    return constant_buffer_entry<T>{*this, add_entry(name, p_data_chunk, entry_size), p_data_chunk->size()};
}

//! helper: adds vector array into the constant buffer
template<typename T>
constant_buffer_entry<std::vector<T>> add_vector_array(std::string const& name, std::vector<T> const& vector_array)
{
    uint8_t const aligned_dimension = T::dimension <= 2U ? T::dimension : 4U;
    size_t const entry_size = aligned_dimension * sizeof(bool_to_uint32<T::value_type>::type) * vector_array.size() - (T:dimension == 3);
    DataChunk* p_data_chunk = new DataChunk{ entry_size };
    constant_buffer_hlsl_type_raw_data_converter<T>::convert(p_data_chunk->data(), vector_array);
    return constant_buffer_entry<std::vector<T>>{*this, add_entry(name, p_data_chunk, T::dimension * sizeof(bool_to_uint32<T::value_type>::type)), p_data_chunk->size()};
}

//! helper: adds new matrix entry into the constant buffer
template<typename T, uint32_t nrows, uint32_t ncolumns>
constant_buffer_entry<math::shader_matrix_type<T, nrows, ncolumns>> add_matrix(std::string const& name,
    math::shader_matrix_type<T, nrows, ncolumns> const& hlsl_matrix)
{
    uint8_t const aligned_column_bucket_size = nrows <= 2U ? nrows : 4U;
    size_t const entry_size = sizeof(bool_to_uint32<T>::type) * aligned_column_bucket_size * ncolumns - (nrows == 3);
    DataChunk* p_data_chunk = new DataChunk{ entry_size };
    constant_buffer_hlsl_type_raw_data_converter<T>::convert(p_data_chunk->data(), hlsl_matrix);
    return constant_buffer_entry<math::shader_matrix_type<T, nrows, ncolumns>>{*this, add_entry(name, p_data_chunk, nrows * sizeof(bool_to_uint32<T>::type)), p_data_chunk->size()};
}

//! helper: adds array of matrices into the constant buffer
template<typename T, uint32_t nrows, uint32_t ncolumns>
constant_buffer_entry<std::vector<math::shader_matrix_type<T, nrows, ncolumns>>> add_matrix_array(std::string const& name,
    std::vector<math::shader_matrix_type<T, nrows, ncolumns>> const& hlsl_matrix_array)
{
    uint8_t const aligned_column_bucket_size = nrows <= 2U ? nrows : 4U;
    size_t const entry_size = sizeof(bool_to_uint32<T>::type) * aligned_column_bucket_size * ncolumns * hlsl_matrix_array.size() - (nrows == 3);
    DataChunk* p_data_chunk = new DataChunk{ entry_size };
    constant_buffer_hlsl_type_raw_data_converter<T>::convert(p_data_chunk->data(), hlsl_matrix_array);
    return constant_buffer_entry<std::vector<math::shader_matrix_type<T, nrows, ncolumns>>>{*this, add_entry(name, p_data_chunk, nrows * sizeof(bool_to_uint32<T>::type)), p_data_chunk->size()};
}