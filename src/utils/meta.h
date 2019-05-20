#pragma once

#include <cassert>
#include <iterator>
#include <tuple>

namespace meta {

template <auto v, typename T>
struct case_t {
    static constexpr auto value = v;
    using type = T;
    using value_type = decltype(v);
};

template <bool b, typename T>
using cond_t = case_t<b, T>;

namespace internal {
template <typename Cond, typename... Rest>
struct select_impl
    : std::enable_if_t<
          std::is_same_v<typename Cond::value_type, bool>,
          std::conditional_t<Cond::value, Cond, select_impl<Rest...>>> {};

template <typename T>
struct select_impl<T> {
    using type = T; // else clause
};

template <bool b, typename T>
struct select_impl<cond_t<b, T>> {
    // last cond, enforce true
    static_assert(b, "ALL OF THE CASES ARE FALSE BUT NO DEFAULT IS GIVEN");
    using type = T;
};

template <auto v, typename T>
struct case_to_cond {
    using type = T;
};
template <auto v, auto vt, typename T>
struct case_to_cond<v, case_t<vt, T>> {
    using type = cond_t<v == vt, T>;
};

template <auto v, typename T>
using case_to_cond_t = typename case_to_cond<v, T>::type;

} // namespace internal

template <typename Case, typename... Rest>
using select_t = typename internal::select_impl<Case, Rest...>::type;

template <auto v, typename Case, typename... Rest>
using switch_t = select_t<internal::case_to_cond_t<v, Case>,
                          internal::case_to_cond_t<v, Rest>...>;

namespace internal {
template <typename T, T Begin, class Func, T... Is>
constexpr void static_for_impl(Func &&f, std::integer_sequence<T, Is...>) {
    (std::forward<Func>(f)(std::integral_constant<T, Begin + Is>{}), ...);
}
} //  namespace internal
template <typename T, T Begin, T End, class Func>
constexpr void static_for(Func &&f) {
    internal::static_for_impl<T, Begin>(
        std::forward<Func>(f), std::make_integer_sequence<T, End - Begin>{});
}

template <typename T, class Func, T... Is>
constexpr auto apply_sequence(Func &&f, std::integer_sequence<T, Is...>)
    -> decltype(auto) {
    return std::forward<Func>(f)(Is...);
}

template <typename T, class Func, T... Is>
constexpr auto apply_const_sequence(Func &&f, std::integer_sequence<T, Is...>)
    -> decltype(auto) {
    return std::forward<Func>(f)(std::integral_constant<T, Is>{}...);
}

template <typename T, typename std::enable_if_t<std::is_integral_v<T>, int> = 0>
constexpr auto bitcount(T value) {
    std::size_t count = 0;
    while (value > 0) {       // until all bits are zero
        if ((value & 1) == 1) // check lower bit
            count++;
        value >>= 1; // shift bits, removing lower bit
    }
    return count;
}

template <typename T, typename std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr auto bitcount(T value) {
    using UT = std::underlying_type_t<T>;
    return bitcount(static_cast<UT>(value));
}

template <typename To, typename From,
          typename = std::enable_if_t<std::is_integral_v<From> &&
                                      std::is_integral_v<To>>>
To size_cast(From value) {
    assert(value == static_cast<From>(static_cast<To>(value)));
    return static_cast<To>(value);
}

// overload pattern
// https://www.bfilipek.com/2018/06/variant.html
template <class... Ts>
struct overload : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overload(Ts...)->overload<Ts...>;

struct nop {
    void operator()(...) const {}
};

template <typename T>
using is_nop = std::is_same<T, nop>;
template <typename T>
inline constexpr bool is_nop_v = is_nop<T>::value;

template <class T>
struct always_false : std::false_type {};

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
template <typename T, typename = void>
struct is_integral_constant : std::false_type {};
template <typename T, T v>
struct is_integral_constant<std::integral_constant<T, v>> : std::true_type {};

template <typename T, typename = void>
struct is_iterable : std::false_type {};
template <typename T>
struct is_iterable<T, std::void_t<decltype(std::declval<T>().begin()),
                                  decltype(std::declval<T>().end())>>
    : std::true_type {};

template <typename T, typename = void>
struct is_sized : std::false_type {};
template <typename T>
struct is_sized<T, std::void_t<decltype(std::declval<T>().size())>>
    : std::true_type {};

template <typename T>
struct is_iterator {
    static char test(...);

    template <typename U,
              typename = typename std::iterator_traits<U>::difference_type,
              typename = typename std::iterator_traits<U>::pointer,
              typename = typename std::iterator_traits<U>::reference,
              typename = typename std::iterator_traits<U>::value_type,
              typename = typename std::iterator_traits<U>::iterator_category>
    static long test(U &&);

    constexpr static bool value =
        std::is_same<decltype(test(std::declval<T>())), long>::value;
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

template <typename T, typename F, typename... Args>
struct rt_is_type {
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

template <typename tuple_t>
constexpr auto t2a(tuple_t &&tuple) {
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

template <typename T, typename = void>
struct has_insert : std::false_type {};
template <typename T>
struct has_insert<T, std::void_t<decltype(std::declval<T>().insert(
                         std::declval<typename T::value_type>()))>>
    : std::true_type {};

template <typename T, typename size_t = std::size_t, typename = void>
struct has_resize : std::false_type {};
template <typename T, typename size_t>
struct has_resize<
    T, size_t,
    std::void_t<decltype(std::declval<T>().resize(std::declval<size_t>()))>>
    : std::true_type {};

template <typename T>
struct scalar_traits {
    using type = typename std::decay_t<T>;
    constexpr static bool value = std::is_arithmetic_v<type>;
};

namespace internal {
template <typename... Ts>
auto fwd_capture_impl(Ts &&... xs) {
    return std::make_tuple(std::forward<decltype(xs)>(xs)...);
}
} // namespace internal

} // namespace meta

// overload macro with number of arguments
// https://stackoverflow.com/a/45600545/1824372
#define VA_BUGFX(x) x
#define VA_NARG2(...) VA_BUGFX(VA_NARG1(__VA_ARGS__, VA_RSEQN()))
#define VA_NARG1(...) VA_BUGFX(VA_ARGSN(__VA_ARGS__))
#define VA_ARGSN(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define VA_RSEQN() 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define VA_FUNC2(name, n) name##n
#define VA_FUNC1(name, n) VA_FUNC2(name, n)
#define GET_MACRO(func, ...)                                                   \
    VA_FUNC1(func, VA_BUGFX(VA_NARG2(__VA_ARGS__)))(__VA_ARGS__)

// https://stackoverflow.com/a/45043324/1824372
// #define VA_SELECT( NAME, NUM ) NAME ## NUM
// #define VA_COMPOSE( NAME, ARGS ) NAME ARGS
// #define VA_GET_COUNT( _0, _1, _2, _3, _4, _5, _6 /* ad nauseam */, COUNT, ...
// ) COUNT #define VA_EXPAND() ,,,,,, // 6 commas (or 7 empty tokens) #define
// VA_SIZE( ... ) VA_COMPOSE( VA_GET_COUNT, (VA_EXPAND __VA_ARGS__ (), 0, 6, 5,
// 4, 3, 2, 1) ) #define GET_MACRO( NAME, ... ) VA_SELECT( NAME,
// VA_SIZE(__VA_ARGS__) )(__VA_ARGS__)

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
#define REQUIRES_(...) std::enable_if_t<(__VA_ARGS__::value), int> = 0
#define REQUIRES_V(...) typename = std::enable_if_t<(__VA_ARGS__)>
#define REQUIRES_RT(...)                                                       \
    std::enable_if_t<(__VA_ARGS__::value), typename __VA_ARGS__::type>

#define SIZET(...) meta::size_cast<std::size_t>(__VA_ARGS__)

#define FWD(...) std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

#define DECAY(x) std::decay_t<decltype(x)>

#define LIFT1(f)                                                               \
    [](auto &&... xs) noexcept(noexcept(f(FWD(xs)...)))                        \
        ->decltype(f(FWD(xs)...)) {                                            \
        return f(FWD(xs)...);                                                  \
    }
#define LIFT2(o, f)                                                            \
    [&o](auto &&... xs) noexcept(noexcept(o.f(FWD(xs)...)))                    \
        ->decltype(o.f(FWD(xs)...)) {                                          \
        return o.f(FWD(xs)...);                                                \
    }
#define LIFT(...) GET_MACRO(LIFT, __VA_ARGS__)

#define define_has_member_traits(class_name, member_name)                      \
    class has_##member_name {                                                  \
        typedef char yes_type;                                                 \
        typedef long no_type;                                                  \
        template <typename U>                                                  \
        static yes_type test(decltype(&U::member_name));                       \
        template <typename U>                                                  \
        static no_type test(...);                                              \
                                                                               \
    public:                                                                    \
        static constexpr bool value =                                          \
            sizeof(test<class_name>(0)) == sizeof(yes_type);                   \
    }

#define BOOLT(...) std::integral_constant<bool, __VA_ARGS__>
