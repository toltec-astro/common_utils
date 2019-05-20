#pragma once

#include "bits.h"
#include "enum/bitmask.h"
#include "enum/meta_enum.h"
#include "meta.h"

#include <climits>
#include <variant>

namespace enum_utils {

namespace internal {

/// @brief Return the mask value ( values | ...) of enum type.
template <typename T, REQUIRES_(std::is_enum<T>)>
constexpr auto get_mask() {
    using U = std::underlying_type_t<T>;
    U mask{0};
    if constexpr (decltype(enum_has_meta(meta_enum::type_t<T>{}))::value) {
        // build value mask using meta members
        using meta_t = decltype(enum_meta_type(meta_enum::type_t<T>{}));
        for (const auto &meta : meta_t::members) {
            mask |= static_cast<U>(meta.value);
        }
    } else if constexpr (constexpr auto value = bitmask::bitmask<T>::mask_value;
                         value > 0) {
        // use bitmask mask value if defined as bitmask
        mask = value;
    } else {
        // no mask defined for T
    }
    return mask;
};

/// @brief Return the number of set bits for enum
template <typename T, REQUIRES_(std::is_enum<T>)>
constexpr auto bitcount(T v) {
    return bits_utils::count(static_cast<std::underlying_type_t<T>>(v));
}

} // namespace internal

/// @brief The mask value ( values | ...) of enum type.
template <typename T, REQUIRES_(std::is_enum<T>)>
inline constexpr auto bitmask_v = internal::get_mask<T>();

/// @brief The number of used bits in enum type
template <typename T, REQUIRES_(std::is_enum<T>)>
inline constexpr auto bitwidth_v = bits_utils::fls(bitmask_v<T>);

/// @brief Check if enum value has multiple bits set
template <auto v, REQUIRES_(std::is_enum<decltype(v)>)>
inline constexpr auto is_compound_v = (internal::bitcount(v) > 1);

/// @brief Decompose enum value to an array of enum values
template <auto v, REQUIRES_(std::is_enum<decltype(v)>)>
constexpr auto decompose() {
    using T = decltype(v);
    using U = std::underlying_type_t<T>;
    return meta::apply_sequence(
        [](auto &&... i) { return std::array{static_cast<T>(i)...}; },
        bits_utils::decompose<static_cast<U>(v)>());
}

template <auto v, template <decltype(v), typename...> class TT,
          REQUIRES_(std::is_enum<decltype(v)>)>
constexpr auto enum_to_variant() {
    // decompose
    using T = decltype(v);
    using U = std::underlying_type_t<T>;
    return meta::apply_const_sequence(
        [](auto... i) {
            return std::variant<TT<static_cast<T>(decltype(i)::value)>...>{};
        },
        bits_utils::decompose<static_cast<U>(v)>());
}

template <auto v, template <decltype(v), typename...> class TT,
          REQUIRES_(std::is_enum<decltype(v)>)>
using enum_to_variant_t = decltype(enum_to_variant<v, TT>());

} // namespace enum_utils

#define BITMASK_(Type, UnderlyingType, ValueMask, ...)                         \
    enum class Type : UnderlyingType { __VA_ARGS__ };                          \
    BITMASK_DETAIL_DEFINE_VALUE_MASK(Type, ValueMask)                          \
    BITMASK_DETAIL_DEFINE_OPS(Type)                                            \
    META_ENUM_IMPL(Type, UnderlyingType, __VA_ARGS__)

#define BITMASK(Type, UnderlyingType, ValueMask, ...)                          \
    enum class Type : UnderlyingType { __VA_ARGS__ };                          \
    BITMASK_DETAIL_DEFINE_VALUE_MASK(Type, ValueMask)                          \
    BITMASK_DETAIL_DEFINE_OPS_INLINE(Type)                                     \
    META_ENUM_IMPL(Type, UnderlyingType, __VA_ARGS__)
