#ifndef LEXGINE_RUNTIME_IS_CALL_POSSIBLE_H
#define LEXGINE_RUNTIME_IS_CALL_POSSIBLE_H

#define DECLARE_IS_CALL_SUPPORTED_REFLEXION(api_name) \
template<typename T, typename U> \
class is_api_##api_name##_supported; \
\
template<typename T, typename R, typename... Args> \
class is_api_##api_name##_supported<T, R(Args...)> \
{ \
    template<typename U> \
    static constexpr std::true_type _deduce(R(U::*)(Args...)) \
    { \
        return std::true_type{}; \
    } \
\
    template<typename U> \
    static constexpr std::false_type _deduce(U) \
    { \
        return std::false_type{}; \
    } \
\
    template<typename U> \
    static constexpr decltype(_deduce(static_cast<R(U::*)(Args...)>(&U::api_name))) _deduce(R(U::*)(Args...), int) \
    { \
        return decltype(_deduce(static_cast<R(U::*)(Args...)>(&U::api_name))){}; \
    } \
\
    template<typename U> \
    static constexpr std::false_type _deduce(float, float) \
    { \
        return std::true_type{}; \
    } \
\
public: \
    static constexpr bool value = decltype(_deduce<T>(0, 0))::value; \
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
    static constexpr std::true_type _deduce(R(U::*)(Args...) const) \
    { \
        return std::true_type{}; \
    } \
\
    template<typename U> \
    static constexpr std::false_type _deduce(U) \
    { \
        return std::false_type{}; \
    } \
\
    template<typename U> \
    static constexpr decltype(_deduce(static_cast<R(U::*)(Args...) const>(&U::api_name))) _deduce(R(U::*)(Args...) const, int) \
    { \
        return decltype(_deduce(static_cast<R(U::*)(Args...) const>(&U::api_name))){}; \
    } \
\
    template<typename U> \
    static constexpr std::false_type _deduce(float, float) \
    { \
        return std::true_type{}; \
    } \
\
public: \
    static constexpr bool value = decltype(_deduce<T>(0, 0))::value; \
}

#endif