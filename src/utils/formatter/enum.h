#pragma once
#include "../bitmask.h"
#include "../meta_enum.h"
#include "container.h"
#include <fmt/format.h>

namespace fmt {

template <typename T> struct formatter<bitmask::bitmask<T>> {

    // d: the value
    // s: the string
    // l: the string and value
    char spec = 'l';
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx) -> decltype(ctx.begin()) {
        auto it = ctx.begin(), end = ctx.end();
        if (it == end)
            return it;
        if ((*it == 'd') || (*it == 's') || (*it == 'l')) {
            spec = *it++;
        }
        return it;
    }

    template <typename FormatContext>
    auto format(const bitmask::bitmask<T> &bm, FormatContext &ctx)
        -> decltype(ctx.out()) {
        auto it = ctx.out();
        if (spec == 'd') {
            return format_to(it, "{:b}", bm.bits());
        }
        if ((spec == 's') || (spec == 'l')) {
            if constexpr (decltype(has_meta(meta_enum::type_t<T>{}))::value) {
                return format_with_meta(it, bm);
            } else {
                return format_as_bits(it, bm);
            }
        }
        return it;
    }

    using UnderlyingType = typename std::underlying_type<T>::type;
    template <typename FormatContextOut>
    auto format_with_meta(FormatContextOut &it, const bitmask::bitmask<T> &bm) {
        using me = decltype(enum_meta(meta_enum::type_t<T>{}));
        auto bm_meta = me::from_value(static_cast<T>(bm));
        if (bm_meta) {
            if (spec == 's') {
                return format_to(it, "{}", bm_meta.value().name);
            }
            if (spec == 'l') {
                return format_to(it, "{}", bm_meta);
            }
        }
        // composed type, decompose
        auto mask = (bm.mask_value >> 1) + 1;
        it = format_to(it, "(");
        bool sep = false;
        while (mask) {
            if (auto b = mask & bm.bits(); b) {
                auto v = me::from_value(static_cast<T>(b)).value();
                it = format_to(it, "{}{}", sep ? "|" : "", v.name);
                sep = true;
            }
            mask >>= 1;
        }
        if (spec == 's') {
            return format_to(it, ")");
        }
        if (spec == 'l') {
            return format_to(it, ",{:b})", bm.bits());
        }
        return format_to(it, ")");
    }
    template <typename FormatContextOut>
    auto format_as_bits(FormatContextOut &it, const bitmask::bitmask<T> &bm) {
        return format_to(it, "({:b})", bm.bits());
    }

};

template <typename EnumType, typename UnderlyingTypeIn, size_t size>
struct formatter<meta_enum::MetaEnum<EnumType, UnderlyingTypeIn, size>> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx) -> decltype(ctx.begin()) {
        auto it = ctx.begin(), end = ctx.end();
        if (it == end)
            return it;
        return it;
    }

    template <typename FormatContext>
    auto format(const meta_enum::MetaEnum<EnumType, UnderlyingTypeIn, size> &em,
                FormatContext &ctx) -> decltype(ctx.out()) {
        auto it = ctx.out();
        auto str = std::string(em.string);
        str.erase(std::remove_if(str.begin(), str.end(),
                                 [](const auto &c) { return std::isspace(c); }),
                  str.end());
        if (str.empty()) {
            return format_to(it, "{}({})", em.name,
                             static_cast<UnderlyingTypeIn>(em.value));
        }
        return format_to(it, "{}({})", em.name, str);
    }
};

template <typename EnumType>
struct formatter<meta_enum::MetaEnumMember<EnumType>>
    : formatter<bitmask::bitmask<EnumType>> {
    template <typename FormatContext>
    auto format(const meta_enum::MetaEnumMember<EnumType> &em,
                FormatContext &ctx) -> decltype(ctx.out()) {
        if (this->spec == 'l') {
            auto it = ctx.out();
            auto str = std::string(em.string);
            str.erase(
                std::remove_if(str.begin(), str.end(),
                               [](const auto &c) { return std::isspace(c); }),
                str.end());
            str.erase(str.begin(), ++std::find(str.begin(), str.end(), '='));
            if (str.empty()) {
                return format_to(it, "{}({})", em.name,
                                 static_cast<int>(em.value));
            }
            return format_to(it, "{}({})", em.name, str);
        }
        if constexpr (bitmask::bitmask_detail::has_value_mask<
                          EnumType>::value) {
            return formatter<bitmask::bitmask<EnumType>>::format(
                bitmask::bitmask<EnumType>{em.value}, ctx);
        } else {
            return format_to(ctx.out(), "{}", em.name);
        }
    }
};

} // namespace fmt
