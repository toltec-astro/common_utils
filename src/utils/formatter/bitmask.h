#pragma once
#include "../bitmask.h"
#include "../meta.h"
#include "../meta_enum.h"
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
            return format_as_str(it, bm);
        }
        return it;
    }

    using UnderlyingType = typename std::underlying_type<T>::type;

    template <typename FormatContextOut, typename U = T,
              typename = std::enable_if_t<
                  decltype(has_meta(meta_enum::type_t<U>{}))::value>>
    auto format_as_str(FormatContextOut &it, const bitmask::bitmask<T> &bm) {
        using meta = decltype(enum_meta(meta_enum::type_t<U>{}));
        auto bm_meta = meta::from_value(static_cast<T>(bm));
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
                auto v = meta::from_value(static_cast<T>(b)).value();
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
            return format_to(it, "{}({})", em.name, str);
        }
        return formatter<bitmask::bitmask<EnumType>>::format(
            bitmask::bitmask<EnumType>{em.value}, ctx);
    }
};

} // namespace fmt
