#pragma once

#include <tuple>
#include <iterator>

namespace meta {

namespace internal {
template <typename... Ts> auto fwd_capture_impl(Ts &&... xs) {
    return std::make_tuple(std::forward<decltype(xs)>(xs)...);
}
template <typename T, T Begin, class Func, T... Is>
constexpr void static_for_impl(Func &&f, std::integer_sequence<T, Is...>) {
    (std::forward<Func>(f)(std::integral_constant<T, Begin + Is>{}), ...);
}
} // namespace internal

template <typename T, T Begin, T End, class Func>
constexpr void static_for(Func &&f) {
    internal::static_for_impl<T, Begin>(
        std::forward<Func>(f), std::make_integer_sequence<T, End - Begin>{});
}

template <typename Tuple> constexpr auto tuplesize(const Tuple &) {
    return std::tuple_size<Tuple>::value;
}

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>> constexpr auto bitcount(T value) {
    T count = 0;
    while (value > 0) {       // until all bits are zero
        if ((value & 1) == 1) // check lower bit
            count++;
        value >>= 1; // shift bits, removing lower bit
    }
    return count;
}

struct nop {
    void operator()(...) const {}
};

template<typename T>
using is_nop = std::is_same<T, nop>;
template<typename T>
inline constexpr bool is_nop_v = is_nop<T>::value;

template <class T> struct always_false : std::false_type {};

// check if type is template instance
template <typename, template <typename...> typename = std::void_t,
          template <typename...> typename = std::void_t>
struct is_instance : public std::false_type {};

template <typename... Ts, template <typename...> typename T>
struct is_instance<T<Ts...>, T> : public std::true_type {};

template <typename... Ts, template <typename...> typename T,
          template <typename...> typename U, typename... Us>
struct is_instance<T<U<Us...>, Ts...>, T, U> : public std::true_type {};

// some useful type traits
template <typename T, typename = void> struct is_iterable : std::false_type {};
template <typename T>
struct is_iterable<T, std::void_t<decltype(std::declval<T>().begin()),
                                  decltype(std::declval<T>().end())>>
    : std::true_type {};

template <typename T, typename = void> struct is_sized : std::false_type {};
template <typename T>
struct is_sized<T, std::void_t<decltype(std::declval<T>().size())>>
    : std::true_type {};

template <typename T>
struct is_iterator {
  static char test(...);

  template <typename U,
    typename=typename std::iterator_traits<U>::difference_type,
    typename=typename std::iterator_traits<U>::pointer,
    typename=typename std::iterator_traits<U>::reference,
    typename=typename std::iterator_traits<U>::value_type,
    typename=typename std::iterator_traits<U>::iterator_category
  > static long test(U&&);

  constexpr static bool value = std::is_same<decltype(test(std::declval<T>())),long>::value;
};


// return type traits for functors
template <template <typename...> typename traits, typename F, typename... Args>
struct rt_has_traits {
    using type = typename std::invoke_result<F, Args...>::type;
    static constexpr bool value = traits<type>::value;
};

template <template <typename...> typename T, template <typename...> typename U,
          typename F, typename... Args>
struct rt_is_instance {
    using type = typename std::invoke_result<F, Args...>::type;
    static constexpr bool value = is_instance<type, T, U>::value;
};

template <template <typename...> typename T, typename F, typename... Args>
struct rt_is_instance<T, std::void_t, F, Args...> {
    using type = typename std::invoke_result<F, Args...>::type;
    static constexpr bool value = is_instance<type, T>::value;
};

template <typename T, typename F, typename... Args> struct rt_is_type {
    using type = typename std::invoke_result<F, Args...>::type;
    static constexpr bool value = std::is_same<type, T>::value;
};

/*
template<typename,
         template <typename...> typename,
         template <typename...> typename
         >
struct is_nested_instance: public std::false_type{};

template <typename...Ts,
          template <typename...> typename T,
          template <typename...> typename U,
          typename...Us
          >
struct is_nested_instance<T<U<Us...>, Ts...>, T, U>: public std::true_type {};
*/

struct explicit_copy_mixin {
    explicit_copy_mixin() = default;
    ~explicit_copy_mixin() = default;
    explicit_copy_mixin(explicit_copy_mixin &&) = default;
    explicit_copy_mixin &operator=(explicit_copy_mixin &&) = default;
    explicit_copy_mixin copy() { return explicit_copy_mixin{*this}; }

private:
    explicit_copy_mixin(const explicit_copy_mixin &) = default;
    explicit_copy_mixin &operator=(const explicit_copy_mixin &) = default;
};

template <typename tuple_t> constexpr auto t2a(tuple_t &&tuple) {
    constexpr auto get_array = [](auto &&... x) {
        return std::array{std::forward<decltype(x)>(x)...};
    };
    return std::apply(get_array, std::forward<tuple_t>(tuple));
}

template <class T>
struct is_c_str
    : std::integral_constant<
          bool, std::is_same_v<char const *, typename std::decay_t<T>> ||
                    std::is_same_v<char *, typename std::decay_t<T>>> {};

template <typename T, typename = void>
struct has_push_back : std::false_type {};
template <typename T>
struct has_push_back<T, std::void_t<decltype(std::declval<T>().push_back(
                            std::declval<typename T::value_type>()))>>
    : std::true_type {};

template <typename T, typename = void> struct has_insert : std::false_type {};
template <typename T>
struct has_insert<T, std::void_t<decltype(std::declval<T>().insert(
                         std::declval<typename T::value_type>()))>>
    : std::true_type {};

template <typename T, typename size_t = std::size_t, typename = void> struct has_resize : std::false_type {};
template <typename T, typename size_t>
struct has_resize<T, size_t, std::void_t<decltype(std::declval<T>().resize(
                         std::declval<size_t>()))>>
    : std::true_type {};

template <typename T> struct scalar_traits {
    using type = typename std::decay_t<T>;
    constexpr static bool value =
        std::is_arithmetic_v<type>;
};

} // namespace meta

// overload macro with number of arguments
// https://stackoverflow.com/a/45600545/1824372
#define __BUGFX(x) x
#define __NARG2(...) __BUGFX(__NARG1(__VA_ARGS__, __RSEQN()))
#define __NARG1(...) __BUGFX(__ARGSN(__VA_ARGS__))
#define __ARGSN(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define __RSEQN() 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define __FUNC2(name, n) name##n
#define __FUNC1(name, n) __FUNC2(name, n)
#define GET_MACRO(func, ...)                                                   \
    __FUNC1(func, __BUGFX(__NARG2(__VA_ARGS__)))(__VA_ARGS__)

// https://stackoverflow.com/a/45043324/1824372
// #define VA_SELECT( NAME, NUM ) NAME ## NUM
// #define VA_COMPOSE( NAME, ARGS ) NAME ARGS
// #define VA_GET_COUNT( _0, _1, _2, _3, _4, _5, _6 /* ad nauseam */, COUNT, ... ) COUNT
// #define VA_EXPAND() ,,,,,, // 6 commas (or 7 empty tokens)
// #define VA_SIZE( ... ) VA_COMPOSE( VA_GET_COUNT, (VA_EXPAND __VA_ARGS__ (), 0, 6, 5, 4, 3, 2, 1) )
// #define GET_MACRO( NAME, ... ) VA_SELECT( NAME, VA_SIZE(__VA_ARGS__) )(__VA_ARGS__)

#define FWD(...) std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

#define FWD_CAPTURE(...) GET_MACRO(FWD_CAPTURE, __VA_ARGS__)
#define FWD_CAPTURE1(x1) meta::internal::fwd_capture_impl(FWD(x1))
#define FWD_CAPTURE2(x1, x2) meta::internal::fwd_capture_impl(FWD(x1), FWD(x2))
#define FWD_CAPTURE3(x1, x2, x3)                                               \
    meta::internal::fwd_capture_impl(FWD(x1), FWD(x2), FWD(x3))
#define FWD_CAPTURE4(x1, x2, x3, x4)                                           \
    meta::internal::fwd_capture_impl(FWD(x1), FWD(x2), FWD(x3), FWD(x4))
#define FWD_CAPTURE5(x1, x2, x3, x4, x5)                                       \
    meta::internal::fwd_capture_impl(FWD(x1), FWD(x2), FWD(x3), FWD(x4),       \
                                     FWD(x5))

#define REQUIRES(...) typename = std::enable_if_t<(__VA_ARGS__::value)>
#define REQUIRES_V(...) typename = std::enable_if_t<(__VA_ARGS__)>
#define REQUIRES_RT(...)                                                       \
    std::enable_if_t<(__VA_ARGS__::value), typename __VA_ARGS__::type>
