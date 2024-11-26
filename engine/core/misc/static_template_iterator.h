#ifndef LEXGINE_CORE_MISC_STATIC_TEMPLATE_ITERATOR_H
#define LEXGINE_CORE_MISC_STATIC_TEMPLATE_ITERATOR_H

namespace lexgine::core::misc {

#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>

template <typename... Args>
struct TypeContainer;

template <>
struct TypeContainer<> {
    template <int>
    using get = void;

    static consteval size_t size() { return 0; }
};

template <typename Head, typename... Tail>
struct TypeContainer<Head, Tail...> : TypeContainer<Tail...> {
    template <int element_id>
    using get = std::conditional<element_id == 0, Head, typename TypeContainer<Tail...>::template get<element_id - 1>>::type;

    static consteval size_t size() { return 1 + sizeof...(Tail); }
};

template <typename T, T... Vals>
struct ValueContainer;

template <>
struct ValueContainer<void> { };

template <typename T>
struct ValueContainer<T> : ValueContainer<void> {
    using value_type = T;

    template <int>
    static constexpr T get {};

    static consteval size_t size() { return 0; }
};

template <typename T, T Head, T... Tail>
struct ValueContainer<T, Head, Tail...> : ValueContainer<T, Tail...> {
    template <int element_id>
    static constexpr T get { element_id == 0 ? Head : ValueContainer<T, Tail...>::template get<element_id - 1> };

    static consteval size_t size() { return 1 + sizeof...(Tail); }
};

template <typename CType, int element_id, bool is_value_type = std::is_base_of<ValueContainer<void>, CType>::value>
struct WrappedContainer;

template <typename CType, int element_id>
struct WrappedContainer<CType, element_id, false> {
    using value = TypeContainer<typename CType::template get<element_id>>;
};

template <typename CType, int element_id>
struct WrappedContainer<CType, element_id, true> {
    using value = ValueContainer<typename CType::value_type, CType::template get<element_id>>;
};

template <typename... CArgs>
struct Container;

template <>
struct Container<> {
    template <int>
    using getContainer = void;

    static consteval std::integer_sequence<int> getNextIterationIndex(std::integer_sequence<int> s) { return s; }
};

template <typename CHead, typename... CTail>
struct Container<CHead, CTail...> : Container<CTail...> {
    template <int container_id>
    using getContainer = typename std::conditional<container_id == 0, CHead, typename Container<CTail...>::template getContainer<container_id - 1>>::type;

    template <int container_id>
    using fetch = getContainer<container_id>::template get<0>;

    static consteval size_t variantCount()
    {
        return (CHead::size() * ... * CTail::size());
    }

    template <int newInt, int... Ints>
    static consteval auto attachIndex(std::integer_sequence<int, Ints...>)
    {
        return std::integer_sequence<int, newInt, Ints...> {};
    }

    template <int HeadInt, int... TailInts>
    static consteval auto getNextIterationIndex(std::integer_sequence<int, HeadInt, TailInts...>)
    {
        if constexpr (HeadInt < CHead::size() - 1) {
            return std::integer_sequence<int, HeadInt + 1, TailInts...> {};
        } else {
            return attachIndex<0>(Container<CTail...>::getNextIterationIndex(std::integer_sequence<int, TailInts...> {}));
        }
    }

    template <int... Ints>
    static consteval auto generateFirstIterationIndex(std::integer_sequence<int, Ints...> s)
    {
        if constexpr (sizeof...(Ints) < sizeof...(CTail) + 1) {
            return generateFirstIterationIndex(attachIndex<0>(s));
        } else {
            return s;
        }
    }

    template <int... Ints>
    static consteval bool isFirstIterationIndex(std::integer_sequence<int, Ints...>)
    {
        return (Ints | ...) == 0;
    }

    template <int... Ints>
    static consteval bool isLastIterationIndex(std::integer_sequence<int, Ints...> s)
    {
        auto ns = getNextIterationIndex(s);
        return isFirstIterationIndex(ns);
    }

    template <template <typename> typename Foo, int HeadInt, int... TailInts>
    static bool iterateHelper(std::integer_sequence<int, HeadInt, TailInts...> s)
    {
        using CType = Container<typename WrappedContainer<CHead, HeadInt>::value, typename WrappedContainer<CTail, TailInts>::value...>;
        bool invocation_result = Foo<CType>::iterate();

        if constexpr (isLastIterationIndex(s)) {
            return true;
        } else {
            return invocation_result ? iterateHelper<Foo>(getNextIterationIndex(s)) : false;
        }
    }

    template <template <typename> typename Foo>
    static bool iterate()
    {
        return iterateHelper<Foo>(generateFirstIterationIndex(std::integer_sequence<int, 0> {}));
    }

    template<template <typename> typename Foo, typename T, int HeadInt, int ... TailInts>
    static bool iterateHelper(T&& user_data, std::integer_sequence<int, HeadInt, TailInts...> s)
    {
        using CType = Container<typename WrappedContainer<CHead, HeadInt>::value, typename WrappedContainer<CTail, TailInts>::value...>;
        bool invocation_result = (Foo<CType>::iterate(std::forward<T>(user_data)));

        if constexpr (isLastIterationIndex(s)) {
            return true;
        } else {
            return invocation_result ? iterateHelper<Foo>(std::forward<T>(user_data), getNextIterationIndex(s)) : false;
        }
    }

    template <template <typename> typename Foo, typename T>
    static bool iterate(T&& user_data)
    {
        return iterateHelper<Foo>(std::forward<T>(user_data), generateFirstIterationIndex(std::integer_sequence<int, 0> {}));
    }
};


template<typename ... CArgs>
using StaticTemplateIterator = Container<CArgs...>;


#define FETCH_TYPE(T, id) typename T::template getContainer<id>::template get<0>
#define FETCH_VALUE(T, id) T::template getContainer<id>::template get<0>


}


#endif