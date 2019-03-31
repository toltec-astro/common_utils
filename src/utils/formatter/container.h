#pragma once
#include <fmt/format.h>
#include <optional>
#include <variant>
// list of stl containers is from https://stackoverflow.com/a/31105859/1824372
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace fmt_utils {

// specialize a type for all of the STL containers.
namespace internal {

// list of stl containers is from https://stackoverflow.com/a/31105859/1824372
template <typename T> struct is_stl_container : std::false_type {};
template <typename T, std::size_t N>
struct is_stl_container<std::array<T, N>> : std::true_type {};
template <typename... Args>
struct is_stl_container<std::vector<Args...>> : std::true_type {};
template <typename... Args>
struct is_stl_container<std::deque<Args...>> : std::true_type {};
template <typename... Args>
struct is_stl_container<std::list<Args...>> : std::true_type {};
template <typename... Args>
struct is_stl_container<std::forward_list<Args...>> : std::true_type {};
template <typename... Args>
struct is_stl_container<std::set<Args...>> : std::true_type {};
template <typename... Args>
struct is_stl_container<std::multiset<Args...>> : std::true_type {};
template <typename... Args>
struct is_stl_container<std::map<Args...>> : std::true_type {};
template <typename... Args>
struct is_stl_container<std::multimap<Args...>> : std::true_type {};
template <typename... Args>
struct is_stl_container<std::unordered_set<Args...>> : std::true_type {};
template <typename... Args>
struct is_stl_container<std::unordered_multiset<Args...>> : std::true_type {};
template <typename... Args>
struct is_stl_container<std::unordered_map<Args...>> : std::true_type {};
template <typename... Args>
struct is_stl_container<std::unordered_multimap<Args...>> : std::true_type {};
template <typename... Args>
struct is_stl_container<std::stack<Args...>> : std::true_type {};
template <typename... Args>
struct is_stl_container<std::queue<Args...>> : std::true_type {};
template <typename... Args>
struct is_stl_container<std::priority_queue<Args...>> : std::true_type {};

} // namespace internal

template <typename T>
using is_stl_container = internal::is_stl_container<std::decay_t<T>>;

template <class T>
struct is_c_str
    : std::integral_constant<
          bool, std::is_same_v<char const *, typename std::decay_t<T>> ||
                    std::is_same_v<char *, typename std::decay_t<T>>> {};

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

template <typename T, typename U> struct formatter<std::pair<T, U>> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx) -> decltype(ctx.begin()) {
        auto it = ctx.begin(), end = ctx.end();
        if (it == end) {
            return it;
        }
        return it;
    }
    template <typename FormatContext>
    auto format(const std::pair<T, U> &p, FormatContext &ctx)
        -> decltype(ctx.out()) {
        return format_to(ctx.out(), "{{{}: {}}}", p.first, p.second);
    }
};

/*
template <typename T, typename Char, template <typename...> typename U,
          typename... Rest>
struct formatter<
    U<T, Rest...>, Char,
    std::enable_if_t<
        // stl containers
        fmt_utils::is_stl_container<U<T, Rest...>>::value
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
        >> {
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
*/

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
                std::string t;
                std::string f = "{0:} ({1:})";
                using A = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<A, bool>) {
                    t = "bool";
                } else if constexpr (std::is_same_v<A, int>) {
                    t = "int";
                } else if constexpr (std::is_same_v<A, double>) {
                    t = "doub";
                } else if constexpr (std::is_same_v<A, std::string> ||
                                     fmt_utils::is_c_str<A>::value) {
                    t = "str";
                    f = "\"{0:}\" ({1:})";
                } else {
                    t = "?";
                    // static_assert(meta::always_false<T>::value, "NOT KNOWN
                    // TYPE");
                }
                it = format_to(it, f, arg, t);
            },
            v);
        return it;
    }
};

} // namespace fmt

namespace std {

/// provide output stream operator for all containers
template <typename OStream, typename T, typename... Rest,
          template <typename...> typename U,
          typename = std::enable_if_t<
              // stl containers
              fmt_utils::is_stl_container<U<T, Rest...>>::value>>
OStream &operator<<(OStream &os, const U<T, Rest...> &cont) {
    os << "{";
    bool sep = false;
    for (const auto &v : cont) {
        if (sep) {
            os << ", ";
        }
        os << fmt::format("{}", v);
    }
    os << "}";
    return os;
}

} // namespace std
