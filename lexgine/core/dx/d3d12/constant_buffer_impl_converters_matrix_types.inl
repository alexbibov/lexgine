// Specializations for HLSL matrix types

template<typename T> class constant_buffer_hlsl_type_raw_data_converter<math::shader_matrix_type<T, 2, 2>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, math::shader_matrix_type<T, 2, 2> const& matrix2x2)
    {
        T* p_aux = static_cast<T*>(p_raw_data_buffer);

        memcpy(p_aux, matrix2x2.getRawData(), sizeof(T) * 4);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static math::shader_matrix_type<T, 2, 2> unconvert(void const* p_raw_data_buffer)
    {
        T const* p_aux = static_cast<T const*>(p_raw_data_buffer);
        shader_matrix_type<T, 2, 2> rv;

        memcpy(rv.getRawData(), p_aux, sizeof(T) * 4);

        return rv;
    }
};

template<> class constant_buffer_hlsl_type_raw_data_converter<math::shader_matrix_type<bool, 2, 2>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void* p_raw_data_buffer, math::shader_matrix_type<bool, 2, 2> const& matrix2x2)
    {
        uint32_t* p_aux = static_cast<uint32_t*>(p_raw_data_buffer);
        bool const* p_matrix_data = matrix2x2.getRawData();

        for (uint8_t i = 0; i < 4U; ++i) p_aux[i] = static_cast<uint32_t>(p_matrix_data[i]);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static math::shader_matrix_type<bool, 2, 2> unconvert(void const* p_raw_data_buffer)
    {
        uint32_t const* p_aux = static_cast<uint32_t const*>(p_raw_data_buffer);
        math::shader_matrix_type<bool, 2, 2> rv;
        bool* p_matrix_data = rv.getRawData();
        for (uint8_t i = 0; i < 4U; ++i) p_matrix_data[i] = p_aux[i] != 0;
        return rv;
    }
};


template<typename T> class constant_buffer_hlsl_type_raw_data_converter<math::shader_matrix_type<T, 2, 3>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, math::shader_matrix_type<T, 2, 3> const& matrix2x3)
    {
        T* p_aux = static_cast<T*>(p_raw_data_buffer);
        memcpy(p_aux, matrix2x3.getRawData(), sizeof(T) * 6);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static math::shader_matrix_type<T, 2, 3> unconvert(void const* p_raw_data_buffer)
    {
        T const* p_aux = static_cast<T const*>(p_raw_data_buffer);
        shader_matrix_type<T, 2, 3> rv;
        memcpy(rv.getRawData(), p_aux, sizeof(T) * 6);
        return rv;
    }
};

template<> class constant_buffer_hlsl_type_raw_data_converter<math::shader_matrix_type<bool, 2, 3>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void* p_raw_data_buffer, math::shader_matrix_type<bool, 2, 3> const& matrix2x3)
    {
        uint32_t* p_aux = static_cast<uint32_t*>(p_raw_data_buffer);
        bool const* p_matrix_data = matrix2x3.getRawData();
        for (uint8_t i = 0; i < 6U; ++i) p_aux[i] = static_cast<uint32_t>(p_matrix_data[i]);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static math::shader_matrix_type<bool, 2, 3> unconvert(void const* p_raw_data_buffer)
    {
        uint32_t const* p_aux = static_cast<uint32_t const*>(p_raw_data_buffer);
        math::shader_matrix_type<bool, 2, 3> rv;
        bool* p_matrix_data = rv.getRawData();
        for (uint8_t i = 0; i < 6U; ++i) p_matrix_data[i] = p_aux[i] != 0;
        return rv;
    }
};


template<typename T> class constant_buffer_hlsl_type_raw_data_converter<math::shader_matrix_type<T, 2, 4>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, math::shader_matrix_type<T, 2, 4> const& matrix2x4)
    {
        T* p_aux = static_cast<T*>(p_raw_data_buffer);
        memcpy(p_aux, matrix2x4.getRawData(), sizeof(T) * 8);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static math::shader_matrix_type<T, 2, 4> unconvert(void const* p_raw_data_buffer)
    {
        T const* p_aux = static_cast<T const*>(p_raw_data_buffer);
        shader_matrix_type<T, 2, 4> rv;
        memcpy(rv.getRawData(), p_aux, sizeof(T) * 8);
        return rv;
    }
};

template<> class constant_buffer_hlsl_type_raw_data_converter<math::shader_matrix_type<bool, 2, 4>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void* p_raw_data_buffer, math::shader_matrix_type<bool, 2, 4> const& matrix2x4)
    {
        uint32_t* p_aux = static_cast<uint32_t*>(p_raw_data_buffer);
        bool const* p_matrix_data = matrix2x4.getRawData();
        for (uint8_t i = 0; i < 8U; ++i) p_aux[i] = static_cast<uint32_t>(p_matrix_data[i]);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static math::shader_matrix_type<bool, 2, 4> unconvert(void const* p_raw_data_buffer)
    {
        uint32_t const* p_aux = static_cast<uint32_t const*>(p_raw_data_buffer);
        math::shader_matrix_type<bool, 2, 4> rv;
        bool* p_matrix_data = rv.getRawData();
        for (uint8_t i = 0; i < 8U; ++i) p_matrix_data[i] = p_aux[i] != 0;
        return rv;
    }
};




template<typename T> class constant_buffer_hlsl_type_raw_data_converter<math::shader_matrix_type<T, 3, 2>>
{
public:
    using value_type = bool_to_uint32<T>::type;

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, math::shader_matrix_type<T, 3, 2> const& matrix3x2)
    {
        value_type* p_aux = static_cast<value_type*>(p_raw_data_buffer);
        T const* p_matrix_data = matrix3x2.getRawData();

        // First column
        p_aux[0] = static_cast<value_type>(p_matrix_data[0]);
        p_aux[1] = static_cast<value_type>(p_matrix_data[1]);
        p_aux[2] = static_cast<value_type>(p_matrix_data[2]);

        // Second column
        p_aux[4] = static_cast<value_type>(p_matrix_data[3]);
        p_aux[5] = static_cast<value_type>(p_matrix_data[4]);
        p_aux[6] = static_cast<value_type>(p_matrix_data[5]);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static math::shader_matrix_type<T, 3, 2> unconvert(void const* p_raw_data_buffer)
    {
        value_type const* p_aux = static_cast<value_type const*>(p_raw_data_buffer);
        shader_matrix_type<T, 3, 2> rv;
        T* p_matrix_data = rv.getRawData();

        // First column
        p_matrix_data[0] = static_cast<T>(p_aux[0]);
        p_matrix_data[1] = static_cast<T>(p_aux[1]);
        p_matrix_data[2] = static_cast<T>(p_aux[2]);

        // Second column
        p_matrix_data[3] = static_cast<T>(p_aux[4]);
        p_matrix_data[4] = static_cast<T>(p_aux[5]);
        p_matrix_data[5] = static_cast<T>(p_aux[6]);

        return rv;
    }
};


template<typename T> class constant_buffer_hlsl_type_raw_data_converter<math::shader_matrix_type<T, 3, 3>>
{
public:
    using value_type = bool_to_uint32<T>::type;

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, math::shader_matrix_type<T, 3, 3> const& matrix3x3)
    {
        value_type* p_aux = static_cast<value_type*>(p_raw_data_buffer);
        T const* p_matrix_data = matrix3x3.getRawData();

        // First column
        p_aux[0] = static_cast<value_type>(p_matrix_data[0]);
        p_aux[1] = static_cast<value_type>(p_matrix_data[1]);
        p_aux[2] = static_cast<value_type>(p_matrix_data[2]);

        // Second column
        p_aux[4] = static_cast<value_type>(p_matrix_data[3]);
        p_aux[5] = static_cast<value_type>(p_matrix_data[4]);
        p_aux[6] = static_cast<value_type>(p_matrix_data[5]);

        // Third column
        p_aux[8] = static_cast<value_type>(p_matrix_data[6]);
        p_aux[9] = static_cast<value_type>(p_matrix_data[7]);
        p_aux[10] = static_cast<value_type>(p_matrix_data[8]);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static math::shader_matrix_type<T, 3, 3> unconvert(void const* p_raw_data_buffer)
    {
        value_type const* p_aux = static_cast<value_type const*>(p_raw_data_buffer);
        shader_matrix_type<T, 3, 3> rv;
        T* p_matrix_data = rv.getRawData();

        // First column
        p_matrix_data[0] = static_cast<T>(p_aux[0]);
        p_matrix_data[1] = static_cast<T>(p_aux[1]);
        p_matrix_data[2] = static_cast<T>(p_aux[2]);

        // Second column
        p_matrix_data[3] = static_cast<T>(p_aux[4]);
        p_matrix_data[4] = static_cast<T>(p_aux[5]);
        p_matrix_data[5] = static_cast<T>(p_aux[6]);

        // Third column
        p_matrix_data[6] = static_cast<T>(p_aux[8]);
        p_matrix_data[7] = static_cast<T>(p_aux[9]);
        p_matrix_data[8] = static_cast<T>(p_aux[10]);

        return rv;
    }
};


template<typename T> class constant_buffer_hlsl_type_raw_data_converter<math::shader_matrix_type<T, 3, 4>>
{
public:
    using value_type = bool_to_uint32<T>::type;

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, math::shader_matrix_type<T, 3, 4> const& matrix3x4)
    {
        value_type* p_aux = static_cast<value_type*>(p_raw_data_buffer);
        T const* p_matrix_data = matrix3x4.getRawData();

        // First column
        p_aux[0] = static_cast<value_type>(p_matrix_data[0]);
        p_aux[1] = static_cast<value_type>(p_matrix_data[1]);
        p_aux[2] = static_cast<value_type>(p_matrix_data[2]);

        // Second column
        p_aux[4] = static_cast<value_type>(p_matrix_data[3]);
        p_aux[5] = static_cast<value_type>(p_matrix_data[4]);
        p_aux[6] = static_cast<value_type>(p_matrix_data[5]);

        // Third column
        p_aux[8] = static_cast<value_type>(p_matrix_data[6]);
        p_aux[9] = static_cast<value_type>(p_matrix_data[7]);
        p_aux[10] = static_cast<value_type>(p_matrix_data[8]);

        // Fourth column
        p_aux[12] = static_cast<value_type>(p_matrix_data[9]);
        p_aux[13] = static_cast<value_type>(p_matrix_data[10]);
        p_aux[14] = static_cast<value_type>(p_matrix_data[11]);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static math::shader_matrix_type<T, 3, 4> unconvert(void const* p_raw_data_buffer)
    {
        value_type const* p_aux = static_cast<value_type const*>(p_raw_data_buffer);
        shader_matrix_type<T, 3, 4> rv;
        T* p_matrix_data = rv.getRawData();

        // First column
        p_matrix_data[0] = static_cast<T>(p_aux[0]);
        p_matrix_data[1] = static_cast<T>(p_aux[1]);
        p_matrix_data[2] = static_cast<T>(p_aux[2]);

        // Second column
        p_matrix_data[3] = static_cast<T>(p_aux[4]);
        p_matrix_data[4] = static_cast<T>(p_aux[5]);
        p_matrix_data[5] = static_cast<T>(p_aux[6]);

        // Third column
        p_matrix_data[6] = static_cast<T>(p_aux[8]);
        p_matrix_data[7] = static_cast<T>(p_aux[9]);
        p_matrix_data[8] = static_cast<T>(p_aux[10]);

        // Fourth column
        p_matrix_data[9] = static_cast<T>(p_aux[12]);
        p_matrix_data[10] = static_cast<T>(p_aux[13]);
        p_matrix_data[11] = static_cast<T>(p_aux[14]);

        return rv;
    }
};




template<typename T> class constant_buffer_hlsl_type_raw_data_converter<math::shader_matrix_type<T, 4, 2>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, math::shader_matrix_type<T, 4, 2> const& matrix4x2)
    {
        T* p_aux = static_cast<T*>(p_raw_data_buffer);
        T const* p_matrix_data = matrix4x2.getRawData();
        memcpy(p_aux, p_matrix_data, sizeof(T) * 8);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static math::shader_matrix_type<T, 4, 2> unconvert(void const* p_raw_data_buffer)
    {
        T const* p_aux = static_cast<T const*>(p_raw_data_buffer);
        shader_matrix_type<T, 4, 2> rv;
        T* p_matrix_data = rv.getRawData();

        memcpy(p_matrix_data, p_aux, sizeof(T) * 8);

        return rv;
    }
};

template<> class constant_buffer_hlsl_type_raw_data_converter<math::shader_matrix_type<bool, 4, 2>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, math::shader_matrix_type<bool, 4, 2> const& matrix4x2)
    {
        uint32_t* p_aux = static_cast<uint32_t*>(p_raw_data_buffer);
        bool const* p_matrix_data = matrix4x2.getRawData();
        for (uint8_t i = 0; i < 8U; ++i) p_aux[i] = static_cast<uint32_t>(p_matrix_data[i]);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static math::shader_matrix_type<bool, 4, 2> unconvert(void const* p_raw_data_buffer)
    {
        uint32_t const* p_aux = static_cast<uint32_t const*>(p_raw_data_buffer);
        math::shader_matrix_type<bool, 4, 2> rv;
        bool* p_matrix_data = rv.getRawData();
        for (uint8_t i = 0; i < 8U; ++i) p_matrix_data[i] = p_aux[i] != 0;
        return rv;
    }
};


template<typename T> class constant_buffer_hlsl_type_raw_data_converter<math::shader_matrix_type<T, 4, 3>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, math::shader_matrix_type<T, 4, 3> const& matrix4x3)
    {
        T* p_aux = static_cast<T*>(p_raw_data_buffer);
        T const* p_matrix_data = matrix4x3.getRawData();
        memcpy(p_aux, p_matrix_data, sizeof(T) * 12);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static math::shader_matrix_type<T, 4, 3> unconvert(void const* p_raw_data_buffer)
    {
        T const* p_aux = static_cast<T const*>(p_raw_data_buffer);
        shader_matrix_type<T, 4, 3> rv;
        T* p_matrix_data = rv.getRawData();

        memcpy(p_matrix_data, p_aux, sizeof(T) * 12);

        return rv;
    }
};

template<> class constant_buffer_hlsl_type_raw_data_converter<math::shader_matrix_type<bool, 4, 3>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, math::shader_matrix_type<bool, 4, 3> const& matrix4x3)
    {
        uint32_t* p_aux = static_cast<uint32_t*>(p_raw_data_buffer);
        bool const* p_matrix_data = matrix4x3.getRawData();
        for (uint8_t i = 0; i < 12U; ++i) p_aux[i] = static_cast<uint32_t>(p_matrix_data[i]);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static math::shader_matrix_type<bool, 4, 3> unconvert(void const* p_raw_data_buffer)
    {
        uint32_t const* p_aux = static_cast<uint32_t const*>(p_raw_data_buffer);
        math::shader_matrix_type<bool, 4, 3> rv;
        bool* p_matrix_data = rv.getRawData();
        for (uint8_t i = 0; i < 12U; ++i) p_matrix_data[i] = p_aux[i] != 0;
        return rv;
    }
};


template<typename T> class constant_buffer_hlsl_type_raw_data_converter<math::shader_matrix_type<T, 4, 4>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, math::shader_matrix_type<T, 4, 4> const& matrix4x4)
    {
        T* p_aux = static_cast<T*>(p_raw_data_buffer);
        T const* p_matrix_data = matrix4x4.getRawData();
        memcpy(p_aux, p_matrix_data, sizeof(T) * 16);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static math::shader_matrix_type<T, 4, 4> unconvert(void const* p_raw_data_buffer)
    {
        T const* p_aux = static_cast<T const*>(p_raw_data_buffer);
        shader_matrix_type<T, 4, 4> rv;
        T* p_matrix_data = rv.getRawData();

        memcpy(p_matrix_data, p_aux, sizeof(T) * 16);

        return rv;
    }
};

template<> class constant_buffer_hlsl_type_raw_data_converter<math::shader_matrix_type<bool, 4, 4>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, math::shader_matrix_type<bool, 4, 4> const& matrix4x4)
    {
        uint32_t* p_aux = static_cast<uint32_t*>(p_raw_data_buffer);
        bool const* p_matrix_data = matrix4x4.getRawData();
        for (uint8_t i = 0; i < 16U; ++i) p_aux[i] = static_cast<uint32_t>(p_matrix_data[i]);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static math::shader_matrix_type<bool, 4, 4> unconvert(void const* p_raw_data_buffer)
    {
        uint32_t const* p_aux = static_cast<uint32_t const*>(p_raw_data_buffer);
        math::shader_matrix_type<bool, 4, 4> rv;
        bool* p_matrix_data = rv.getRawData();
        for (uint8_t i = 0; i < 16U; ++i) p_matrix_data[i] = p_aux[i] != 0;
        return rv;
    }
};






// Specializations for HLSL arrays of matrix types

template<typename T> class constant_buffer_hlsl_type_raw_data_converter<std::vector<math::shader_matrix_type<T, 2, 2>>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<math::shader_matrix_type<T, 2, 2>> const& matrix2x2_array)
    {
        T* p_aux = static_cast<T*>(p_raw_data_buffer);
        for (size_t i = 0; i < matrix2x2_array.size(); ++i) memcpy(p_aux + 4 * i, matrix2x2_array[i].getRawData(), sizeof(T) * 4);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<math::shader_matrix_type<T, 2, 2>> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % (sizeof(T) * 4) == 0);

        T const* p_aux = static_cast<T const*>(p_raw_data_buffer);

        std::vector<math::shader_matrix_type<T, 2, 2>>::size_type num_elements =
            static_cast<std::vector<math::shader_matrix_type<T, 2, 2>>::size_type>(size_of_raw_data_in_bytes / (sizeof(T) * 4));
        std::vector<math::shader_matrix_type<T, 2, 2>> rv(num_elements);

        for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i) memcpy(rv[i].getRawData(), p_aux + 4 * i, sizeof(T) * 4);

        return rv;
    }
};

template<> class constant_buffer_hlsl_type_raw_data_converter<std::vector<math::shader_matrix_type<bool, 2, 2>>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<math::shader_matrix_type<bool, 2, 2>> const& matrix2x2_array)
    {
        uint32_t* p_aux = static_cast<uint32_t*>(p_raw_data_buffer);
        for (size_t i = 0; i < matrix2x2_array.size(); ++i)
        {
            bool const* p_matrix_data = matrix2x2_array[i].getRawData();
            for (uint8_t j = 0; j < 4U; ++j) p_aux[i * 4 + j] = static_cast<uint32_t>(p_matrix_data[j]);
        }
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<math::shader_matrix_type<bool, 2, 2>> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % (sizeof(uint32_t) * 4) == 0);

        uint32_t const* p_aux = static_cast<uint32_t const*>(p_raw_data_buffer);

        std::vector<math::shader_matrix_type<bool, 2, 2>>::size_type num_elements =
            static_cast<std::vector<math::shader_matrix_type<bool, 2, 2>>::size_type>(size_of_raw_data_in_bytes / (sizeof(uint32_t) * 4));
        std::vector <math::shader_matrix_type<bool, 2, 2>> rv(num_elements);
        for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i)

        {
            bool* p_matrix_data = rv[i].getRawData();
            for (uint8_t j = 0; j < 4U; ++j) p_matrix_data[j] = p_aux[i * 4 + j] != 0;
        }

        return rv;
    }
};


template<typename T> class constant_buffer_hlsl_type_raw_data_converter<std::vector<math::shader_matrix_type<T, 2, 3>>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<math::shader_matrix_type<T, 2, 3>> const& matrix2x3_array)
    {
        T* p_aux = static_cast<T*>(p_raw_data_buffer);
        for (size_t i = 0; i < matrix2x3_array.size(); ++i) memcpy(p_aux + 6 * i, matrix2x3_array[i].getRawData(), sizeof(T) * 6);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<math::shader_matrix_type<T, 2, 3>> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % (sizeof(T) * 6) == 0);

        T const* p_aux = static_cast<T const*>(p_raw_data_buffer);

        std::vector<math::shader_matrix_type<T, 2, 3>>::size_type num_elements =
            static_cast<std::vector<math::shader_matrix_type<T, 2, 3>>::size_type>(size_of_raw_data_in_bytes / (sizeof(T) * 6));
        std::vector<math::shader_matrix_type<T, 2, 3>> rv(num_elements);

        for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i) memcpy(rv[i].getRawData(), p_aux + 6 * i, sizeof(T) * 6);

        return rv;
    }
};

template<> class constant_buffer_hlsl_type_raw_data_converter<std::vector<math::shader_matrix_type<bool, 2, 3>>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<math::shader_matrix_type<bool, 2, 3>> const& matrix2x3_array)
    {
        uint32_t* p_aux = static_cast<uint32_t*>(p_raw_data_buffer);
        for (size_t i = 0; i < matrix2x3_array.size(); ++i)
        {
            bool const* p_matrix_data = matrix2x3_array[i].getRawData();
            for (uint8_t j = 0; j < 6U; ++j) p_aux[i * 6 + j] = static_cast<uint32_t>(p_matrix_data[j]);
        }
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<math::shader_matrix_type<bool, 2, 3>> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % (sizeof(uint32_t) * 6));

        uint32_t const* p_aux = static_cast<uint32_t const*>(p_raw_data_buffer);
        
        std::vector<math::shader_matrix_type<bool, 2, 3>>::size_type num_elements =
            static_cast<std::vector<math::shader_matrix_type<bool, 2, 3>>::size_type>(size_of_raw_data_in_bytes / (sizeof(uint32_t) * 6));
        std::vector <math::shader_matrix_type<bool, 2, 3>> rv(num_elements);
        
        for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i)
        {
            bool* p_matrix_data = rv[i].getRawData();
            for (uint8_t j = 0; j < 6U; ++j) p_matrix_data[j] = p_aux[i * 6 + j] != 0;
        }

        return rv;
    }
};


template<typename T> class constant_buffer_hlsl_type_raw_data_converter<std::vector<math::shader_matrix_type<T, 2, 4>>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<math::shader_matrix_type<T, 2, 4>> const& matrix2x4_array)
    {
        T* p_aux = static_cast<T*>(p_raw_data_buffer);
        for (size_t i = 0; i < matrix2x4_array.size(); ++i) memcpy(p_aux + 8 * i, matrix2x4_array[i].getRawData(), sizeof(T) * 8);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<math::shader_matrix_type<T, 2, 4>> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % (sizeof(T) * 8) == 0);

        T const* p_aux = static_cast<T const*>(p_raw_data_buffer);

        std::vector<math::shader_matrix_type<T, 2, 4>>::size_type num_elements =
            static_cast<std::vector<math::shader_matrix_type<T, 2, 4>>::size_type>(size_of_raw_data_in_bytes / (sizeof(T) * 8));
        std::vector<math::shader_matrix_type<T, 2, 4>> rv(num_elements);

        for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i) memcpy(rv[i].getRawData(), p_aux + 8 * i, sizeof(T) * 8);

        return rv;
    }
};

template<> class constant_buffer_hlsl_type_raw_data_converter<std::vector<math::shader_matrix_type<bool, 2, 4>>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<math::shader_matrix_type<bool, 2, 4>> const& matrix2x4_array)
    {
        uint32_t* p_aux = static_cast<uint32_t*>(p_raw_data_buffer);
        for (size_t i = 0; i < matrix2x4_array.size(); ++i)
        {
            bool const* p_matrix_data = matrix2x4_array[i].getRawData();
            for (uint8_t j = 0; j < 8U; ++j) p_aux[i * 8 + j] = static_cast<uint32_t>(p_matrix_data[j]);
        }
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<math::shader_matrix_type<bool, 2, 4>> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % (sizeof(uint32_t) * 8) == 0);

        uint32_t const* p_aux = static_cast<uint32_t const*>(p_raw_data_buffer);
        
        std::vector<math::shader_matrix_type<bool, 2, 4>>::size_type num_elements =
            static_cast<std::vector<math::shader_matrix_type<bool, 2, 4>>::size_type>(size_of_raw_data_in_bytes / (sizeof(uint32_t) * 8));
        std::vector <math::shader_matrix_type<bool, 2, 4>> rv(num_elements);
        
        for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i)
        {
            bool* p_matrix_data = rv[i].getRawData();
            for (uint8_t j = 0; j < 8U; ++j) p_matrix_data[j] = p_aux[i * 8 + j] != 0;
        }

        return rv;
    }
};




template<typename T> class constant_buffer_hlsl_type_raw_data_converter<std::vector<math::shader_matrix_type<T, 3, 2>>>
{
public:
    using value_type = bool_to_uint32<T>::type;

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<math::shader_matrix_type<T, 3, 2>> const& matrix3x2_array)
    {
        value_type* p_aux = static_cast<value_type*>(p_raw_data_buffer);
        for (size_t i = 0; i < matrix3x2_array.size(); ++i)
        {
            T const* p_matrix_data = matrix3x2_array[i].getRawData();

            // First column
            p_aux[8 * i] = static_cast<value_type>(p_matrix_data[0]);
            p_aux[8 * i + 1] = static_cast<value_type>(p_matrix_data[1]);
            p_aux[8 * i + 2] = static_cast<value_type>(p_matrix_data[2]);

            // Second column
            p_aux[8 * i + 4] = static_cast<value_type>(p_matrix_data[3]);
            p_aux[8 * i + 5] = static_cast<value_type>(p_matrix_data[4]);
            p_aux[8 * i + 6] = static_cast<value_type>(p_matrix_data[5]);
        }
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<math::shader_matrix_type<T, 3, 2>> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % (sizeof(T) * 8) == 0);

        value_type const* p_aux = static_cast<value_type const*>(p_raw_data_buffer);
        
        std::vector<math::shader_matrix_type<T, 3, 2>>::size_type num_elements =
            static_cast<std::vector<math::shader_matrix_type<T, 3, 2>>::size_type>((size_of_raw_data_in_bytes + 1) / (sizeof(T) * 8));
        std::vector<math::shader_matrix_type<T, 3, 2>> rv(num_elements);
        
        for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i)
        {
            T* p_matrix_data = rv[i].getRawData();

            // First column
            p_matrix_data[0] = static_cast<T>(p_aux[8 * i]);
            p_matrix_data[1] = static_cast<T>(p_aux[8 * i + 1]);
            p_matrix_data[2] = static_cast<T>(p_aux[8 * i + 2]);

            // Second column
            p_matrix_data[3] = static_cast<T>(p_aux[8 * i + 4]);
            p_matrix_data[4] = static_cast<T>(p_aux[8 * i + 5]);
            p_matrix_data[5] = static_cast<T>(p_aux[8 * i + 6]);
        }

        return rv;
    }
};


template<typename T> class constant_buffer_hlsl_type_raw_data_converter<std::vector<math::shader_matrix_type<T, 3, 3>>>
{
public:
    using value_type = bool_to_uint32<T>::type;

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<math::shader_matrix_type<T, 3, 3>> const& matrix3x3_array)
    {
        value_type* p_aux = static_cast<value_type*>(p_raw_data_buffer);
        for (size_t i = 0; i < matrix3x3_array.size(); ++i)
        {
            T const* p_matrix_data = matrix3x3_array[i].getRawData();

            // First column
            p_aux[12 * i] = static_cast<value_type>(p_matrix_data[0]);
            p_aux[12 * i + 1] = static_cast<value_type>(p_matrix_data[1]);
            p_aux[12 * i + 2] = static_cast<value_type>(p_matrix_data[2]);

            // Second column
            p_aux[12 * i + 4] = static_cast<value_type>(p_matrix_data[3]);
            p_aux[12 * i + 5] = static_cast<value_type>(p_matrix_data[4]);
            p_aux[12 * i + 6] = static_cast<value_type>(p_matrix_data[5]);

            // Third column
            p_aux[12 * i + 8] = static_cast<value_type>(p_matrix_data[6]);
            p_aux[12 * i + 9] = static_cast<value_type>(p_matrix_data[7]);
            p_aux[12 * i + 10] = static_cast<value_type>(p_matrix_data[8]);
        }
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<math::shader_matrix_type<T, 3, 3>> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % (sizeof(T) * 12) == 0);

        value_type const* p_aux = static_cast<value_type const*>(p_raw_data_buffer);

        std::vector<math::shader_matrix_type<T, 3, 3>>::size_type num_elements =
            static_cast<std::vector<math::shader_matrix_type<T, 3, 3>>::size_type>((size_of_raw_data_in_bytes + 1) / (sizeof(T) * 12));
        std::vector<math::shader_matrix_type<T, 3, 3>> rv(num_elements);

        for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i)
        {
            T* p_matrix_data = rv[i].getRawData();

            // First column
            p_matrix_data[0] = static_cast<T>(p_aux[12 * i]);
            p_matrix_data[1] = static_cast<T>(p_aux[12 * i + 1]);
            p_matrix_data[2] = static_cast<T>(p_aux[12 * i + 2]);

            // Second column
            p_matrix_data[3] = static_cast<T>(p_aux[12 * i + 4]);
            p_matrix_data[4] = static_cast<T>(p_aux[12 * i + 5]);
            p_matrix_data[5] = static_cast<T>(p_aux[12 * i + 6]);

            // Third column
            p_matrix_data[6] = static_cast<T>(p_aux[12 * i + 8]);
            p_matrix_data[7] = static_cast<T>(p_aux[12 * i + 9]);
            p_matrix_data[8] = static_cast<T>(p_aux[12 * i + 10]);
        }

        return rv;
    }
};


template<typename T> class constant_buffer_hlsl_type_raw_data_converter<std::vector<math::shader_matrix_type<T, 3, 4>>>
{
public:
    using value_type = bool_to_uint32<T>::type;

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<math::shader_matrix_type<T, 3, 4>> const& matrix3x4_array)
    {
        value_type* p_aux = static_cast<value_type*>(p_raw_data_buffer);
        for (size_t i = 0; i < matrix3x4_array.size(); ++i)
        {
            T const* p_matrix_data = matrix3x4_array[i].getRawData();

            // First column
            p_aux[16 * i] = static_cast<value_type>(p_matrix_data[0]);
            p_aux[16 * i + 1] = static_cast<value_type>(p_matrix_data[1]);
            p_aux[16 * i + 2] = static_cast<value_type>(p_matrix_data[2]);

            // Second column
            p_aux[16 * i + 4] = static_cast<value_type>(p_matrix_data[3]);
            p_aux[16 * i + 5] = static_cast<value_type>(p_matrix_data[4]);
            p_aux[16 * i + 6] = static_cast<value_type>(p_matrix_data[5]);

            // Third column
            p_aux[16 * i + 8] = static_cast<value_type>(p_matrix_data[6]);
            p_aux[16 * i + 9] = static_cast<value_type>(p_matrix_data[7]);
            p_aux[16 * i + 10] = static_cast<value_type>(p_matrix_data[8]);

            // Fourth column
            p_aux[16 * i + 12] = static_cast<value_type>(p_matrix_data[9]);
            p_aux[16 * i + 13] = static_cast<value_type>(p_matrix_data[10]);
            p_aux[16 * i + 14] = static_cast<value_type>(p_matrix_data[11]);
        }
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<math::shader_matrix_type<T, 3, 4>> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % (sizeof(T) * 16) == 0);

        value_type const* p_aux = static_cast<value_type const*>(p_raw_data_buffer);

        std::vector<math::shader_matrix_type<T, 3, 4>>::size_type num_elements =
            static_cast<std::vector<math::shader_matrix_type<T, 3, 4>>::size_type>((size_of_raw_data_in_bytes + 1) / (sizeof(T) * 16));
        std::vector<math::shader_matrix_type<T, 3, 4>> rv(num_elements);

        for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i)
        {
            T* p_matrix_data = rv[i].getRawData();

            // First column
            p_matrix_data[0] = static_cast<T>(p_aux[16 * i]);
            p_matrix_data[1] = static_cast<T>(p_aux[16 * i + 1]);
            p_matrix_data[2] = static_cast<T>(p_aux[16 * i + 2]);

            // Second column
            p_matrix_data[3] = static_cast<T>(p_aux[16 * i + 4]);
            p_matrix_data[4] = static_cast<T>(p_aux[16 * i + 5]);
            p_matrix_data[5] = static_cast<T>(p_aux[16 * i + 6]);

            // Third column
            p_matrix_data[6] = static_cast<T>(p_aux[16 * i + 8]);
            p_matrix_data[7] = static_cast<T>(p_aux[16 * i + 9]);
            p_matrix_data[8] = static_cast<T>(p_aux[16 * i + 10]);

            // Fourth column
            p_matrix_data[9] = static_cast<T>(p_aux[16 * i + 12]);
            p_matrix_data[10] = static_cast<T>(p_aux[16 * i + 13]);
            p_matrix_data[11] = static_cast<T>(p_aux[16 * i + 14]);
        }

        return rv;
    }
};




template<typename T> class constant_buffer_hlsl_type_raw_data_converter<std::vector<math::shader_matrix_type<T, 4, 2>>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<math::shader_matrix_type<T, 4, 2>> const& matrix4x2_array)
    {
        T* p_aux = static_cast<T*>(p_raw_data_buffer);
        for (size_t i = 0; i < matrix4x2_array.size(); ++i) memcpy(p_aux + 8 * i, matrix4x2_array[i].getRawData(), sizeof(T) * 8);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<math::shader_matrix_type<T, 4, 2>> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % (sizeof(T) * 8) == 0);

        T const* p_aux = static_cast<T const*>(p_raw_data_buffer);

        std::vector<math::shader_matrix_type<T, 4, 2>>::size_type num_elements =
            static_cast<std::vector<math::shader_matrix_type<T, 4, 2>>::size_type>(size_of_raw_data_in_bytes / (sizeof(T) * 8));
        std::vector<math::shader_matrix_type<T, 4, 2>> rv(num_elements);

        for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i)
        {
            T* p_matrix_data = rv[i].getRawData();
            memcpy(p_matrix_data, p_aux + 8 * i, sizeof(T) * 8);
        }
    }
};

template<> class constant_buffer_hlsl_type_raw_data_converter<std::vector<math::shader_matrix_type<bool, 4, 2>>>
{
    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<math::shader_matrix_type<bool, 4, 2>> const& matrix4x2_array)
    {
        uint32_t* p_aux = static_cast<uint32_t*>(p_raw_data_buffer);
        for (size_t i = 0; i < matrix4x2_array.size(); ++i)
        {
            bool const* p_matrix_data = matrix4x2_array[i].getRawData();
            for (uint8_t j = 0; j < 8U; ++j) p_aux[8 * i + j] = static_cast<uint32_t>(p_matrix_data[j]);
        }
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<math::shader_matrix_type<bool , 4, 2>> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % (sizeof(uint32_t) * 8) == 0);

        uint32_t const* p_aux = static_cast<uint32_t const*>(p_raw_data_buffer);
        
        std::vector<math::shader_matrix_type<bool, 4, 2>>::size_type num_elements =
            static_cast<std::vector<math::shader_matrix_type<bool, 4, 2>>::size_type>(size_of_raw_data_in_bytes / (sizeof(uint32_t) * 8));
        std::vector<math::shader_matrix_type<bool, 4, 2>> rv(num_elements);
        
        for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i)
        {
            bool* p_matrix_data = rv[i].getRawData();
            for (uint8_t j = 0; j < 8U; ++j) p_matrix_data[j] = p_aux[8 * i + j] != 0;
        }
    }
};


template<typename T> class constant_buffer_hlsl_type_raw_data_converter<std::vector<math::shader_matrix_type<T, 4, 3>>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<math::shader_matrix_type<T, 4, 3>> const& matrix4x3_array)
    {
        T* p_aux = static_cast<T*>(p_raw_data_buffer);
        for (size_t i = 0; i < matrix4x3_array.size(); ++i) memcpy(p_aux + 12 * i, matrix4x3_array[i].getRawData(), sizeof(T) * 12);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<math::shader_matrix_type<T, 4, 3>> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % (sizeof(T) * 12) == 0);

        T const* p_aux = static_cast<T const*>(p_raw_data_buffer);

        std::vector<math::shader_matrix_type<T, 4, 3>>::size_type num_elements =
            static_cast<std::vector<math::shader_matrix_type<T, 4, 3>>::size_type>(size_of_raw_data_in_bytes / (sizeof(T) * 12));
        std::vector<math::shader_matrix_type<T, 4, 3>> rv(num_elements);

        for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i)
        {
            T* p_matrix_data = rv[i].getRawData();
            memcpy(p_matrix_data, p_aux + 12 * i, sizeof(T) * 12);
        }
    }
};

template<> class constant_buffer_hlsl_type_raw_data_converter<std::vector<math::shader_matrix_type<bool, 4, 3>>>
{
    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<math::shader_matrix_type<bool, 4, 3>> const& matrix4x3_array)
    {
        uint32_t* p_aux = static_cast<uint32_t*>(p_raw_data_buffer);
        for (size_t i = 0; i < matrix4x3_array.size(); ++i)
        {
            bool const* p_matrix_data = matrix4x3_array[i].getRawData();
            for (uint8_t j = 0; j < 12U; ++j) p_aux[12 * i + j] = static_cast<uint32_t>(p_matrix_data[j]);
        }
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<math::shader_matrix_type<bool, 4, 3>> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % (sizeof(uint32_t) * 12) == 0);

        uint32_t const* p_aux = static_cast<uint32_t const*>(p_raw_data_buffer);
        
        std::vector<math::shader_matrix_type<bool, 4, 3>>::size_type num_elements =
            static_cast<std::vector<math::shader_matrix_type<bool, 4, 3>>::size_type>(size_of_raw_data_in_bytes / (sizeof(uint32_t) * 12));
        std::vector<math::shader_matrix_type<bool, 4, 3>> rv(num_elements);
        
        for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i)
        {
            bool* p_matrix_data = rv[i].getRawData();
            for (uint8_t j = 0; j < 12U; ++j) p_matrix_data[j] = p_aux[12 * i + j] != 0;
        }
    }
};


template<typename T> class constant_buffer_hlsl_type_raw_data_converter<std::vector<math::shader_matrix_type<T, 4, 4>>>
{
public:

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<math::shader_matrix_type<T, 4, 4>> const& matrix4x4_array)
    {
        T* p_aux = static_cast<T*>(p_raw_data_buffer);
        for (size_t i = 0; i < matrix4x4_array.size(); ++i) memcpy(p_aux + 16 * i, matrix4x4_array[i].getRawData(), sizeof(T) * 16);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<math::shader_matrix_type<T, 4, 4>> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % (sizeof(T) * 16) == 0);

        T const* p_aux = static_cast<T const*>(p_raw_data_buffer);

        std::vector<math::shader_matrix_type<T, 4, 4>>::size_type num_elements =
            static_cast<std::vector<math::shader_matrix_type<T, 4, 4>>::size_type>(size_of_raw_data_in_bytes / (sizeof(T) * 16));
        std::vector<math::shader_matrix_type<T, 4, 4>> rv(num_elements);

        for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i)
        {
            T* p_matrix_data = rv[i].getRawData();
            memcpy(p_matrix_data, p_aux + 16 * i, sizeof(T) * 16);
        }
    }
};

template<> class constant_buffer_hlsl_type_raw_data_converter<std::vector<math::shader_matrix_type<bool, 4, 4>>>
{
    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<math::shader_matrix_type<bool, 4, 4>> const& matrix4x4_array)
    {
        uint32_t* p_aux = static_cast<uint32_t*>(p_raw_data_buffer);
        for (size_t i = 0; i < matrix4x4_array.size(); ++i)
        {
            bool const* p_matrix_data = matrix4x4_array[i].getRawData();
            for (uint8_t j = 0; j < 16U; ++j) p_aux[16 * i + j] = static_cast<uint32_t>(p_matrix_data[j]);
        }
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<math::shader_matrix_type<bool, 4, 4>> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % (sizeof(uint32_t) * 16) == 0);

        uint32_t const* p_aux = static_cast<uint32_t const*>(p_raw_data_buffer);
        
        std::vector<math::shader_matrix_type<bool, 4, 4>>::size_type num_elements =
            static_cast<std::vector<math::shader_matrix_type<bool, 4, 4>>::size_type>(size_of_raw_data_in_bytes / (sizeof(uint32_t) * 16));
        std::vector<math::shader_matrix_type<bool, 4, 4>> rv(num_elements);
        
        for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i)
        {
            bool* p_matrix_data = rv[i].getRawData();
            for (uint8_t j = 0; j < 16U; ++j) p_matrix_data[j] = p_aux[16 * i + j] != 0;
        }
    }
};