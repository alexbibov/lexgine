#ifndef LEXGINE_CORE_MISC_TEMPLATE_ARGUMENT_ITERATOR_H

#include <cstdint>

namespace lexgine { namespace core { namespace misc {

    template<typename ... Args> struct arg_pack;

    template<typename Head, typename ... Tail>
    struct arg_pack<Head, Tail...>
    {
        static size_t const size = sizeof...(Tail) + 1U;
        using value_type = Head;
        using next_pack = arg_pack<Tail...>;
        static bool const is_value_pack = false;
    };

    template<typename Head>
    struct arg_pack<Head>
    {
        static size_t const size = 1U;
        using value_type = Head;
        using next_pack = void;
        static bool const is_value_pack = false;
    };


    template<typename T, T HeadValue, T ... TailValues>
    struct value_arg_pack
    {
        static size_t const size = sizeof...(TailValues) + 1U;
        static T const value = HeadValue;
        using value_type = T;
        using next_pack = value_arg_pack<T, TailValues...>;
        static bool const is_value_pack = true;
    };

    template<typename T, T HeadValue>
    struct value_arg_pack<T, HeadValue>
    {
        static size_t const size = 1U;
        static T const value = HeadValue;
        using value_type = T;
        using next_pack = void;
        static bool const is_value_pack = true;
    };


    template<uint32_t index, typename arg_pack_type>
    struct get_type_in_arg_pack_by_index
    {
        using value_type = typename get_type_in_arg_pack_by_index<index - 1, typename arg_pack_type::next_pack>::value_type;
    };

    template<typename arg_pack_type>
    struct get_type_in_arg_pack_by_index<0, arg_pack_type>
    {
        using value_type = typename arg_pack_type::value_type;
    };

    template<uint32_t index, typename Head, typename ... Args> 
    struct get_element_in_variadic_by_index
    {
        using value_type = typename get_element_in_variadic_by_index<index - 1, Args...>::value_type;
    };

    template<typename Head, typename ... Args>
    struct get_element_in_variadic_by_index<0, Head, Args...>
    {
        using value_type = Head;
    };



    



    template<uint32_t index, int32_t cumsum, int32_t idx, typename HeadArgPack, typename ... TailArgPacks>
    struct arg_pack_index_locator
    {
        static uint32_t const index = index - cumsum >= 0 ? arg_pack_index_locator<idx + 1, cumsum + HeadArgPack::size, TailArgPacks...>::index : idx;
    };



    template<typename HeadArgPack, typename ... TailArgPacks>
    class TemplateArgumentIterator
    {
    private:

        template<uint32_t arg_pack_index, uint32_t element_in_arg_pack_index>
        using item_by_index_type = typename get_type_in_arg_pack_by_index<element_in_arg_pack_index,
            typename get_element_in_pack_by_index<arg_pack_index, HeadArgPack, TailArgPacks...>::value_type>::value_type;

        template<uint32_t index>
        struct convert_index
        {
            static int32_t const arg_pack_index = arg_pack_index_locator<index, HeadArgPack::size, 0, TailArgPacks...>::index;
            static int32_t const element_index = index - arg_pack_index_locator<index, HeadArgPack::size, 0, TailArgPacks...>::index;
        };

    public:
        static size_t const size = HeadArgPack::size + TemplateArgumentIterator<TailArgPacks...>::size;

        template<uint32_t index>
        using value_type = item_by_index_type<convert_index<index>::arg_pack_index, convert_index<index>::element_index>;
    };

}}}

#define LEXGINE_CORE_MISC_TEMPLATE_ARGUMENT_ITERATOR_H
#endif
