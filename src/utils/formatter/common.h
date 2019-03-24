#pragma once
#include "../meta.h"
#include <Eigen/Core>
#include <fmt/format.h>
#include <optional>
#include <variant>

namespace fmt_utils {

/// @brief Convert integer type to string with given base.
template <typename T, typename = std::enable_if_t<std::is_unsigned_v<T>>>
auto itoa(T n, unsigned base) {
    assert(base >= 2 && base <= 64);
    const char digits[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+-";
    std::string retval;
    while (n != 0) {
        retval += digits[n % base];
        n /= base;
    }
    std::reverse(retval.begin(), retval.end());
    return retval;
};

/// @brief Convert pointer to the memory address
template <typename T> struct ptr {
    using value_t = std::uintptr_t;
    const value_t value;
    ptr(const T *p) : value(reinterpret_cast<value_t>(p)) {}
};

} // namespace fmt_utils

namespace fmt {

template <typename T> struct formatter<std::optional<T>> : formatter<T> {
    template <typename FormatContext>
    auto format(const std::optional<T> &opt, FormatContext &ctx)
        -> decltype(ctx.out()) {
        if (opt) {
            return formatter<T>::format(opt.value(), ctx);
        }
        return format_to(ctx.out(), "(nullopt)");
    }
};

template <typename T, typename Char, typename... Rest,
          template <typename...> typename U>
struct formatter<
    U<T, Rest...>, Char,
    std::enable_if_t<
        meta::is_iterable<U<T, Rest...>>::value &&
        // exclude eigen type and vector with scalar type, which are handled by
        // matrix formatter
        !(std::is_base_of_v<Eigen::EigenBase<U<T, Rest...>>, U<T, Rest...>>)&&!(
            meta::is_instance<U<T, Rest...>, std::vector>::value &&
            meta::scalar_traits<T>::value) &&
        // exclude string types
        !(meta::is_instance<U<T, Rest...>, std::basic_string>::value ||
          meta::is_instance<U<T, Rest...>, std::basic_string_view>::value ||
          meta::is_instance<U<T, Rest...>, fmt::basic_string_view>::value)>

    > {
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx) -> decltype(ctx.begin()) {
        auto it = ctx.begin(), end = ctx.end();
        if (it == end) {
            return it;
        }
        return it;
    }

    template <typename FormatContext>
    auto format(const U<T, Rest...> &vec, FormatContext &ctx)
        -> decltype(ctx.out()) {
        auto it = ctx.out();
        it = format_to(it, "{{");
        bool sep = false;
        for (const auto &v : vec) {
            if (sep) {
                it = format_to(it, ", ");
            }
            it = format_to(it, "{}", v);
            sep = true;
        }
        return format_to(it, "}}");
    }
};

template <typename T, typename... Rest>
struct formatter<std::variant<T, Rest...>, char, void> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx) -> decltype(ctx.begin()) {
        auto it = ctx.begin(), end = ctx.end();
        if (it == end) {
            return it;
        }
        return it;
    }

    template <typename FormatContext>
    auto format(const std::variant<T, Rest...> &v, FormatContext &ctx)
        -> decltype(ctx.out()) {
        auto it = ctx.out();
        std::visit(
            [&](auto &&arg) {
                std::string t = "";
                using A = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<A, bool>) {
                    t = "bool";
                } else if constexpr (std::is_same_v<A, int>) {
                    t = "int";
                } else if constexpr (std::is_same_v<A, double>) {
                    t = "doub";
                } else if constexpr (std::is_same_v<A, std::string> ||
                                     meta::is_c_str<A>::value) {
                    t = "str";
                } else {
                    t = "???";
                    // static_assert(meta::always_false<T>::value, "NOT KNOWN
                    // TYPE");
                }
                it = format_to(it, "{} [{}]", arg, t);
            },
            v);
        return it;
    }
};

template <typename T> struct formatter<fmt_utils::ptr<T>> {

    // x: base16, i.e., hex
    // y: base32
    // z: base64
    char spec = 'z';

    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx) -> decltype(ctx.begin()) {
        auto it = ctx.begin(), end = ctx.end();
        if (it == end) {
            return it;
        }
        if (*it == 'x' || *it == 'y' || *it == 'z') {
            spec = *it++;
        }
        return it;
    }

    template <typename FormatContext>
    auto format(const fmt_utils::ptr<T> ptr, FormatContext &ctx)
        -> decltype(ctx.out()) {
        auto it = ctx.out();
        switch (spec) {
        case 'x':
            return format_to(it, "{:x}", ptr.value);
        case 'y':
            return format_to(it, "{}", fmt_utils::itoa(ptr.value, 32));
        case 'z':
            return format_to(it, "{}", fmt_utils::itoa(ptr.value, 64));
        }
        return it;
    }
};

} // namespace fmt
