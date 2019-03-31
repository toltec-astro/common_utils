#pragma once
#include <string>
#include <type_traits>

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

} // namespace fmt_utils
