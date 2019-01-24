#ifndef LEXGINE_CORE_MISC_TEMPLATE_ARGUMENT_ITERATOR_H

// Template class implemented below allows to perform compile-time iteration over template parameters.
// For usage example see e.g. d3d12_pso_xml_parser.cpp


#include <cstdint>

namespace lexgine { namespace core { namespace misc {

    template<typename Head, typename ... Tail>
    struct arg_pack
    {
        static size_t const size = sizeof...(Tail) + 1U;
        static int const value = 0;
        using value_type = Head;
        using next_pack = arg_pack<Tail...>;
        static bool const is_value_pack = false;
    };

    template<typename Head>
    struct arg_pack<Head>
    {
        static size_t const size = 1U;
        static int const value = 0;
        using value_type = Head;
        using next_pack = void;
        static bool const is_value_pack = false;
    };


    template<typename T, T HeadValue, T ... TailValues>
    struct value_arg_pack
    {
        static size_t const size = sizeof...(TailValues) + 1U;
        static constexpr T value = HeadValue;
        using value_type = T;
        using next_pack = value_arg_pack<T, TailValues...>;
        static bool const is_value_pack = true;
    };

    template<typename T, T HeadValue>
    struct value_arg_pack<T, HeadValue>
    {
        static size_t const size = 1U;
        static constexpr T value = HeadValue;
        using value_type = T;
        using next_pack = void;
        static bool const is_value_pack = true;
    };



    template<size_t index, typename ArgPackType>
    struct get_type_in_arg_pack_by_index
    {
        using value_type = typename get_type_in_arg_pack_by_index<index - 1, typename ArgPackType::next_pack>::value_type;
    };

    template<typename ArgPackType>
    struct get_type_in_arg_pack_by_index<0, ArgPackType>
    {
        using value_type = typename ArgPackType::value_type;
    };


    template<size_t index, typename ValueArgPackType>
    struct get_value_in_value_arg_pack_by_index
    {
        static typename ValueArgPackType::value_type const value =
            get_value_in_value_arg_pack_by_index<index - 1, typename ValueArgPackType::next_pack>::value;
    };

    template<typename ValueArgPackType>
    struct get_value_in_value_arg_pack_by_index<0, ValueArgPackType>
    {
        static typename ValueArgPackType::value_type const value = ValueArgPackType::value;
    };


    template<typename T, bool is_value_pack>
    struct arg_pack_value_type_converter;

    template<typename T>
    struct arg_pack_value_type_converter<T, true>
    {
        using value_type = T;
    };

    template<typename T>
    struct arg_pack_value_type_converter<T, false>
    {
        using value_type = int;
    };


    template<size_t index, typename GenericArgPackType>
    struct get_value_in_generic_arg_pack_by_index
    {
        using value_type = typename arg_pack_value_type_converter<typename GenericArgPackType::value_type, GenericArgPackType::is_value_pack>::value_type;
        static constexpr value_type value = get_value_in_generic_arg_pack_by_index<index - 1, typename GenericArgPackType::next_pack>::value;
    };

    template<typename GenericArgPackType>
    struct get_value_in_generic_arg_pack_by_index<0, GenericArgPackType>
    {
        using value_type = typename arg_pack_value_type_converter<typename GenericArgPackType::value_type, GenericArgPackType::is_value_pack>::value_type;
        static constexpr value_type value = GenericArgPackType::value;
    };



    template<typename T, typename U, U v, bool has_value>
    struct value_type_and_value_union
    {
        using value_type = T;
        using aux_value_type = U;
        static constexpr aux_value_type value = v;
        static bool const is_value = has_value;
    };

    template<size_t index, typename HeadArgPack, typename ... TailArgPacks>
    struct convert_arg_pack_plane_index_to_argument_tuple_list
    {
        using tuple_type = value_type_and_value_union<
            typename get_type_in_arg_pack_by_index<(index % HeadArgPack::size), HeadArgPack>::value_type,
            typename arg_pack_value_type_converter<typename HeadArgPack::value_type, HeadArgPack::is_value_pack>::value_type,
            get_value_in_generic_arg_pack_by_index<(index % HeadArgPack::size), HeadArgPack>::value,
            HeadArgPack::is_value_pack
            >;
            
        using next_tuple_container_type = convert_arg_pack_plane_index_to_argument_tuple_list<index / HeadArgPack::size, TailArgPacks...>;
    };

    template<size_t index, typename HeadArgPack>
    struct convert_arg_pack_plane_index_to_argument_tuple_list<index, HeadArgPack>
    {
        using tuple_type = value_type_and_value_union<typename get_type_in_arg_pack_by_index<index % HeadArgPack::size, HeadArgPack>::value_type,
            typename arg_pack_value_type_converter<typename HeadArgPack::value_type, HeadArgPack::is_value_pack>::value_type,
            get_value_in_generic_arg_pack_by_index<index % HeadArgPack::size, HeadArgPack>::value, HeadArgPack::is_value_pack>;
    };

    
    template<size_t tuple_element_index, typename ArgumentTupleListType>
    struct get_element_from_argument_tuple_list
    {
        using tuple_type = typename get_element_from_argument_tuple_list<tuple_element_index - 1, typename ArgumentTupleListType::next_tuple_container_type>::tuple_type;
    };

    template<typename ArgumentTupleListType>
    struct get_element_from_argument_tuple_list<0, ArgumentTupleListType>
    {
        using tuple_type = typename ArgumentTupleListType::tuple_type;
    };


    template<typename TupleType, bool is_value_tuple = TupleType::is_value> struct tuple_type_adapter;

    template<typename TupleType> struct tuple_type_adapter<TupleType, false>
    {
        using value_type = typename TupleType::value_type;
    };

    template<typename TupleType> struct tuple_type_adapter<TupleType, true>
    {
        static constexpr typename TupleType::aux_value_type value = TupleType::value;
    };


    template<typename HeadArgPack, typename ... TailArgPacks>
    struct get_number_of_tuples
    {
        static size_t const size = HeadArgPack::size * get_number_of_tuples<TailArgPacks...>::size;
    };
    
    template<typename HeadArgPack>
    struct get_number_of_tuples<HeadArgPack>
    {
        static size_t const size = HeadArgPack::size;
    };

    
    template<typename TupleListType, uint32_t tuple_element_index>
    using get_tuple_element = tuple_type_adapter<typename get_element_from_argument_tuple_list<tuple_element_index, TupleListType>::tuple_type>;


    template<template<typename> class LoopBodyType, typename HeadArgPack, typename ... TailArgPacks>
    class TemplateArgumentIterator
    {
    private:
        template<size_t tuple_index>
        using tuple_list_type = convert_arg_pack_plane_index_to_argument_tuple_list<tuple_index, HeadArgPack, TailArgPacks...>;
        static const size_t number_of_tuples = get_number_of_tuples<HeadArgPack, TailArgPacks...>::size;

        template<size_t index>
        static void loop_iteration(void* user_data)
        {
            if (LoopBodyType<tuple_list_type<index>>::iterate(user_data))
            {
                loop_iteration<index + 1>(user_data);
            }
        }

        template<>
        static void loop_iteration<number_of_tuples - 1>(void* user_data) {}

    public:
        static void loop(void* user_data = nullptr)
        {
            loop_iteration<0>(user_data);
        }
    };

}}}

#define LEXGINE_CORE_MISC_TEMPLATE_ARGUMENT_ITERATOR_H
#endif
