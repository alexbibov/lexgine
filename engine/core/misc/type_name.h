#ifndef LEXGINE_CORE_MISC_TYPE_NAME_H
#define LEXGINE_CORE_MISC_TYPE_NAME_H

#include <string_view>
#include <cstddef>

namespace lexgine::core::misc
{

//! Returns the raw, compiler-specific signature of the enclosing function with the type T baked in.
//! Used as the building block for the compile-time type name extraction below.
template<typename T>
constexpr std::string_view wrappedTypeName()
{
#if defined(__clang__) || defined(__GNUC__)
    return __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
    return __FUNCSIG__;
#else
#error "Compile-time type name extraction is not supported on this compiler"
#endif
}

//! Length of the fixed prefix that the compiler puts in front of the type in wrappedTypeName().
//! Probed using a known type (void) so we do not have to hard-code per-compiler offsets.
inline constexpr std::size_t c_wrapped_type_name_prefix_length =
    wrappedTypeName<void>().find("void");

//! Length of the fixed suffix that the compiler appends after the type in wrappedTypeName().
inline constexpr std::size_t c_wrapped_type_name_suffix_length =
    wrappedTypeName<void>().length() - c_wrapped_type_name_prefix_length - std::string_view{ "void" }.length();

//! Returns the human-readable, fully-qualified name of type T computed entirely at compile time
//! (e.g. "lexgine::scenegraph::Scene"). No run-time cost and no manual bookkeeping required.
template<typename T>
constexpr std::string_view typeName()
{
    constexpr std::string_view wrapped = wrappedTypeName<T>();
    std::string_view name = wrapped.substr(
        c_wrapped_type_name_prefix_length,
        wrapped.length() - c_wrapped_type_name_prefix_length - c_wrapped_type_name_suffix_length);

    // MSVC prefixes class/struct/enum/union types with the corresponding keyword: strip it.
    for (std::string_view keyword : { std::string_view{ "class " }, std::string_view{ "struct " },
        std::string_view{ "enum " }, std::string_view{ "union " } })
    {
        if (name.substr(0, keyword.length()) == keyword)
        {
            name.remove_prefix(keyword.length());
            break;
        }
    }

    return name;
}

}

#endif
