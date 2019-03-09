// Specialization for scalar HLSL types
template<typename T> class constant_buffer_hlsl_type_raw_data_converter
{
public:
    using value_type = bool_to_uint32<T>::type;

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, T scalar_type)
    {
        static_cast<value_type*>(p_raw_data_buffer)[0] = static_cast<value_type>(scalar_type);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static T unconvert(void const* p_raw_data_buffer)
    {
        return static_cast<T>(static_cast<value_type const*>(p_raw_data_buffer)[0]);
    }
};

// Specialization for HLSL arrays of scalar types
template<typename T> class constant_buffer_hlsl_type_raw_data_converter<std::vector<T>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<T> const& array_of_scalars)
    {
        memcpy(p_raw_data_buffer, array_of_scalars.data(), sizeof(T)*array_of_scalars.size());
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<T> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % sizeof(T) == 0);

        std::vector<T>::size_type num_of_elements = static_cast<std::vector<T>::size_type>(size_of_raw_data_in_bytes / sizeof(T));
        T const* p_first_element = static_cast<T const*>(p_raw_data_buffer);
        T const* p_last_element = p_first_element + num_of_elements;
        std::vector<T> rv{ p_first_element, p_last_element };

        return rv;
    }
};

// Specialization for HLSL arrays of booleans
template<> class constant_buffer_hlsl_type_raw_data_converter<std::vector<bool>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<bool> const& array_of_booleans)
    {
        uint32_t* p_aux = static_cast<uint32_t*>(p_raw_data_buffer);
        for (size_t i = 0; i < array_of_booleans.size(); ++i) p_aux[i] = static_cast<uint32_t>(array_of_booleans[i]);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<bool> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % sizeof(uint32_t) == 0);

        uint32_t const* p_aux = static_cast<uint32_t const*>(p_raw_data_buffer);
        std::vector<bool>::size_type num_elements = static_cast<std::vector<bool>::size_type>(size_of_raw_data_in_bytes / sizeof(uint32_t));
        std::vector<bool> rv(num_elements);
        for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i) rv[i] = p_aux[i] != 0;
        return rv;
    }
};