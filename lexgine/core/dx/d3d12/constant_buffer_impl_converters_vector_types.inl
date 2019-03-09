// Specialization for 2D vectors
template<typename T> class constant_buffer_hlsl_type_raw_data_converter<math::tagVector2<T>>
{
public:
    using value_type = bool_to_uint32<T>::type;

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, math::tagVector2<T> const& vector2)
    {
        value_type* p_aux = static_cast<value_type*>(p_raw_data_buffer);
        p_aux[0] = static_cast<value_type>(vector2.x);
        p_aux[1] = static_cast<value_type>(vector2.y);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static math::tagVector2<T> unconvert(void const* p_raw_data_buffer)
    {
        value_type const* p_aux = static_cast<value_type const*>(p_raw_data_buffer);
        return math::tagVector2<T>{static_cast<T>(p_aux[0]), static_cast<T>(p_aux[1])};
    }
};

// Specialization for 3D vectors
template<typename T> class constant_buffer_hlsl_type_raw_data_converter<math::tagVector3<T>>
{
public:
    using value_type = bool_to_uint32<T>::type;

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, math::tagVector3<T> const& vector3)
    {
        value_type* p_aux = static_cast<value_type*>(p_raw_data_buffer);
        p_aux[0] = static_cast<value_type>(vector3.x);
        p_aux[1] = static_cast<value_type>(vector3.y);
        p_aux[2] = static_cast<value_type>(vector3.z);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static math::tagVector3<T> unconvert(void const* p_raw_data_buffer)
    {
        value_type const* p_aux = static_cast<value_type const*>(p_raw_data_buffer);
        return math::tagVector3<T>{static_cast<T>(p_aux[0]), static_cast<T>(p_aux[1]), static_cast<T>(p_aux[2])};
    }
};

// Specialization for 4D vectors
template<typename T> class constant_buffer_hlsl_type_raw_data_converter<math::tagVector4<T>>
{
public:
    using value_type = bool_to_uint32<T>::type;

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, math::tagVector4<T> const& vector4)
    {
        value_type* p_aux = static_cast<value_type*>(p_raw_data_buffer);
        p_aux[0] = static_cast<value_type>(vector4.x);
        p_aux[1] = static_cast<value_type>(vector4.y);
        p_aux[2] = static_cast<value_type>(vector4.z);
        p_aux[3] = static_cast<value_type>(vector4.w);
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static math::tagVector4<T> unconvert(void const* p_raw_data_buffer)
    {
        value_type const* p_aux = static_cast<value_type const*>(p_raw_data_buffer);
        return math::tagVector4<T>{static_cast<T>(p_aux[0]), static_cast<T>(p_aux[1]), static_cast<T>(p_aux[2]), static_cast<T>(p_aux[3])};
    }
};





// Specialization for HLSL arrays of 2D vectors
template<typename T> class constant_buffer_hlsl_type_raw_data_converter<std::vector<math::tagVector2<T>>>
{
public:
    using value_type = bool_to_uint32<T>::type;

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<math::tagVector2<T>> const& vector2_array)
    {
        value_type* p_aux = static_cast<value_type*>(p_raw_data_buffer);
        for (size_t i = 0; i < vector2_array.size(); ++i)
        {
            p_aux[2 * i] = static_cast<value_type>(vector2_array[i].x);
            p_aux[2 * i + 1] = static_cast<value_type>(vector2_array[i].y);
        }
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<math::tagVector2<T>> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % (2 * sizeof(T)) == 0);

        value_type const* p_aux = static_cast<value_type const*>(p_raw_data_buffer);
        std::vector<math::tagVector2<T>>::size_type num_elements =
            static_cast<std::vector<math::tagVector2<T>>::size_type>(size_of_raw_data_in_bytes / (2 * sizeof(T)));
        std::vector<math::tagVector2<T>> rv(num_elements);
        for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i)
        {
            rv[i].x = static_cast<T>(p_aux[2 * i]);
            rv[i].y = static_cast<T>(p_aux[2 * i + 1]);
        }

        return rv;
    }
};

// Specialization for HLSL arrays of 3D vectors
template<typename T> class constant_buffer_hlsl_type_raw_data_converter<std::vector<math::tagVector3<T>>>
{
public:
    using value_type = bool_to_uint32<T>::type;

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<math::tagVector3<T>> const& vector3_array)
    {
        value_type* p_aux = static_cast<value_type*>(p_raw_data_buffer);
        for (size_t i = 0; i < vector3_array.size(); ++i)
        {
            p_aux[4 * i] = static_cast<value_type>(vector3_array[i].x);
            p_aux[4 * i + 1] = static_cast<value_type>(vector3_array[i].y);
            p_aux[4 * i + 2] = static_cast<value_type>(vector3_array[i].z);
        }
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<math::tagVector3<T>> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % (4 * sizeof(T)) == 0);

        value_type const* p_aux = static_cast<value_type const*>(p_raw_data_buffer);
        std::vector<math::tagVector3<T>>::size_type num_elements =
            static_cast<std::vector<math::tagVector3<T>>::size_type>((size_of_raw_data_in_bytes + 1) / (4 * sizeof(T)));
        std::vector<math::tagVector3<T>> rv(num_elements);
        for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i)
        {
            rv[i].x = static_cast<T>(p_aux[4 * i]);
            rv[i].y = static_cast<T>(p_aux[4 * i + 1]);
            rv[i].z = static_cast<T>(p_aux[4 * i + 2]);
        }

        return rv;
    }
};

// Specialization for HLSL arrays of 4D vectors
template<typename T> class constant_buffer_hlsl_type_raw_data_converter<std::vector<math::tagVector4<T>>>
{
public:
    using value_type = bool_to_uint32<T>::type;

    //! converts provided HLSL scalar type to raw data format
    static void convert(void *p_raw_data_buffer, std::vector<math::tagVector4<T>> const& vector4_array)
    {
        value_type* p_aux = static_cast<value_type*>(p_raw_data_buffer);
        for (size_t i = 0; i < vector4_array.size(); ++i)
        {
            p_aux[4 * i] = static_cast<value_type>(vector4_array[i].x);
            p_aux[4 * i + 1] = static_cast<value_type>(vector4_array[i].y);
            p_aux[4 * i + 2] = static_cast<value_type>(vector4_array[i].z);
            p_aux[4 * i + 3] = static_cast<value_type>(vector4_array[i].w);
        }
    }

    //! resolves raw data into the corresponding HLSL type wrapper
    static std::vector<math::tagVector4<T>> unconvert(void const* p_raw_data_buffer, size_t size_of_raw_data_in_bytes)
    {
        assert(size_of_raw_data_in_bytes % (4 * sizeof(T)) == 0);

        value_type const* p_aux = static_cast<value_type const*>(p_raw_data_buffer);
        std::vector<math::tagVector4<T>>::size_type num_elements =
            static_cast<std::vector<math::tagVector4<T>>::size_type>(size_of_raw_data_in_bytes / (4 * sizeof(T)));
        std::vector<math::tagVector4<T>> rv{ num_elements };
        for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i)
        {
            rv[i].x = static_cast<T>(p_aux[4 * i]);
            rv[i].y = static_cast<T>(p_aux[4 * i + 1]);
            rv[i].z = static_cast<T>(p_aux[4 * i + 2]);
            rv[i].w = static_cast<T>(p_aux[4 * i + 3]);
        }

        return rv;
    }
};