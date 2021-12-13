#ifndef LEXGINE_API_EXTERNAL_PARSER_TOKENS_H
#define LEXGINE_API_EXTERNAL_PARSER_TOKENS_H

#define LEXGINE_CPP_API
#define LEXGINE_LUA_API

#define LEXGINE_CALL __cdecl

#ifdef LEXGINE_API_LIB
#define LEXGINE_API __declspec(dllimport)

#include <engine/api/is_call_possible.h>
namespace lexgine::api {

DECLARE_IS_CALL_SUPPORTED_REFLEXION(getNative);

template<typename T, typename = std::enable_if_t<is_api_get_native_supported<T, void* (void)>::value>>
T& unfold(T& obj)
{
    T& temp = *static_cast<T*>(obj.getNative());
    return temp;
}

template<typename T, typename = std::enable_if_t<is_api_get_native_supported_const<T, void const* (void)>::value>>
T const& unfold(T const& obj) { return *static_cast<T const*>(obj.getNative()); }


template<typename T, typename = std::enable_if_t<is_api_get_native_supported<T, void* (void)>::value>>
T* unfold(T* obj) { return static_cast<T*>(obj->getNative()); }

template<typename T, typename = std::enable_if_t<is_api_get_native_supported_const<T, void const* (void)>::value>>
T const* unfold(T const* obj) { return static_cast<T const*>(obj->getNative()); }

template<typename T, typename = std::enable_if_t<is_api_get_native_supported<T, void* (void)>::value>>
T&& unfold(T&& obj) { return std::move(*static_cast<T*>(obj.getNative())); }




template<typename T, typename = std::enable_if_t<!is_api_get_native_supported<T, void* (void)>::value>, typename = void>
inline T* unfold(T* obj) { return obj; }

template<typename T, typename = std::enable_if_t<!is_api_get_native_supported_const<T, void const* (void)>::value>, typename = void>
inline T const* unfold(T const* obj) { return obj; }

template<typename T, typename = std::enable_if_t<!is_api_get_native_supported<T, void* (void)>::value>, typename = void>
inline T& unfold(T& obj)
{
    T& temp = obj;
    return obj;
}

template<typename T, typename = std::enable_if_t<!is_api_get_native_supported_const<T, void const* (void)>::value>, typename = void>
inline T const& unfold(T const& obj) { return obj; }

template<typename T, typename = std::enable_if_t<!is_api_get_native_supported<T, void* (void)>::value>, typename = void>
inline T&& unfold(T&& obj) { return std::move(obj); }

}


template<typename T>
struct public_property_type_accessor
{
    using value_type = T const&;
};

template<typename T>
struct public_property_type_accessor<T&>
{
    using value_type = T&;
};

template<typename T>
struct public_property_type_accessor<T*>
{
    using value_type = T*;
};


template<> struct public_property_type_accessor<bool>
{
    using value_type = bool;
};

template<> struct public_property_type_accessor<char>
{
    using value_type = char;
};

template<> struct public_property_type_accessor<wchar_t>
{
    using value_type = wchar_t;
};

template<> struct public_property_type_accessor<char16_t>
{
    using value_type = char16_t;
};

template<> struct public_property_type_accessor<char32_t>
{
    using value_type = char32_t;
};

template<> struct public_property_type_accessor<short>
{
    using value_type = short;
};

template<> struct public_property_type_accessor<int>
{
    using value_type = int;
};

template<> struct public_property_type_accessor<long>
{
    using value_type = long;
};

template<> struct public_property_type_accessor<long long>
{
    using value_type = long long;
};


template<> struct public_property_type_accessor<unsigned char>
{
    using value_type = unsigned char;
};

template<> struct public_property_type_accessor<unsigned short>
{
    using value_type = unsigned short;
};

template<> struct public_property_type_accessor<unsigned int>
{
    using value_type = unsigned int;
};

template<> struct public_property_type_accessor<unsigned long>
{
    using value_type = unsigned long;
};

template<> struct public_property_type_accessor<unsigned long long>
{
    using value_type = unsigned long long;
};

#else
#define LEXGINE_API __declspec(dllexport)
#endif

#endif