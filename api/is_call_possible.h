#ifndef LEXGINE_RUNTIME_IS_CALL_POSSIBLE_H
#define LEXGINE_RUNTIME_IS_CALL_POSSIBLE_H

#include <type_traits>
#include <utility>

//! Declares is_api_<api_name>_supported<T, R(Args...)> and its _const counterpart, which tell whether
//! T::<api_name>(Args...) is callable and returns exactly R -- on a non-const and on a const lvalue
//! respectively. The detection is expressed in terms of the call expression rather than of a
//! pointer-to-member type: an inherited api_name has the type R(Base::*)(Args...), and forming
//! R(T::*)(Args...) from an overload set is not an exact match, which makes address-of-overloaded-function
//! resolution fail on conforming compilers.
#define DECLARE_IS_CALL_SUPPORTED_REFLEXION(api_name) \
template<typename T, typename U> \
class is_api_##api_name##_supported; \
\
template<typename T, typename R, typename... Args> \
class is_api_##api_name##_supported<T, R(Args...)> \
{ \
    template<typename U> \
    static std::is_same<decltype(std::declval<U&>().api_name(std::declval<Args>()...)), R> _deduce(int); \
\
    template<typename U> \
    static std::false_type _deduce(...); \
\
public: \
    static constexpr bool value = decltype(_deduce<T>(0))::value; \
}; \
\
\
template<typename T, typename R> \
class is_api_##api_name##_supported_const; \
\
template<typename T, typename R, typename... Args> \
class is_api_##api_name##_supported_const<T, R(Args...)> \
{ \
    template<typename U> \
    static std::is_same<decltype(std::declval<U const&>().api_name(std::declval<Args>()...)), R> _deduce(int); \
\
    template<typename U> \
    static std::false_type _deduce(...); \
\
public: \
    static constexpr bool value = decltype(_deduce<T>(0))::value; \
}

#endif
