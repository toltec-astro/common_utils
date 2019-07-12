#pragma once

#include "formatter/byte.h"
#include "formatter/matrix.h"
#include "logging.h"
#include "meta.h"
#include <netcdf>
#include <sstream>
#include <variant>

namespace nc_utils {

namespace internal {

constexpr auto nctypes = std::array{
    netCDF::NcType::ncType::nc_BYTE,  netCDF::NcType::ncType::nc_CHAR,
    netCDF::NcType::ncType::nc_SHORT, netCDF::NcType::ncType::nc_INT,
    netCDF::NcType::ncType::nc_FLOAT, netCDF::NcType::ncType::nc_DOUBLE};

template <auto nctype>
using type_t =
    meta::switch_t<nctype,
                   meta::case_t<netCDF::NcType::ncType::nc_BYTE, std::byte>,
                   meta::case_t<netCDF::NcType::ncType::nc_CHAR, char>,
                   meta::case_t<netCDF::NcType::ncType::nc_SHORT, int16_t>,
                   meta::case_t<netCDF::NcType::ncType::nc_INT, int32_t>,
                   meta::case_t<netCDF::NcType::ncType::nc_FLOAT, float>,
                   meta::case_t<netCDF::NcType::ncType::nc_DOUBLE, double>>;

template <typename T>
inline constexpr auto nctype_v = meta::select_t<
    meta::cond_t<std::is_same_v<std::byte, T>,
                 std::integral_constant<netCDF::NcType::ncType,
                                        netCDF::NcType::ncType::nc_BYTE>>,
    meta::cond_t<std::is_same_v<char, T>,
                 std::integral_constant<netCDF::NcType::ncType,
                                        netCDF::NcType::ncType::nc_CHAR>>,
    meta::cond_t<std::is_same_v<int16_t, T>,
                 std::integral_constant<netCDF::NcType::ncType,
                                        netCDF::NcType::ncType::nc_SHORT>>,
    meta::cond_t<std::is_same_v<int32_t, T>,
                 std::integral_constant<netCDF::NcType::ncType,
                                        netCDF::NcType::ncType::nc_INT>>,
    meta::cond_t<std::is_same_v<float, T>,
                 std::integral_constant<netCDF::NcType::ncType,
                                        netCDF::NcType::ncType::nc_FLOAT>>,
    meta::cond_t<std::is_same_v<double, T>,
                 std::integral_constant<netCDF::NcType::ncType,
                                        netCDF::NcType::ncType::nc_DOUBLE>>>::
    value;

template <std::size_t... Is>
std::variant<type_t<std::get<Is>(nctypes)>...>
type_variant_impl(std::index_sequence<Is...>);
using type_variant_t = decltype(type_variant_impl(
    std::declval<std::make_index_sequence<nctypes.size()>>()));

inline type_variant_t dispatch_as_variant(netCDF::NcType::ncType t) {
    type_variant_t var{};
    using T = netCDF::NcType::ncType;
    switch (t) {
    case T::nc_BYTE: {
        var = type_t<T::nc_BYTE>{};
        break;
    }
    case T::nc_CHAR: {
        var = type_t<T::nc_CHAR>{};
        break;
    }
    case T::nc_SHORT: {
        var = type_t<T::nc_SHORT>{};
        break;
    }
    case T::nc_INT: {
        var = type_t<T::nc_INT>{};
        break;
    }
    case T::nc_FLOAT: {
        var = type_t<T::nc_FLOAT>{};
        break;
    }
    case T::nc_DOUBLE: {
        var = type_t<T::nc_DOUBLE>{};
        break;
    }
    default: {
        throw std::runtime_error(
            fmt::format("dispatch of type {} not implemented",
                        netCDF::NcType{t}.getName()));
    }
    }
    return var;
}

} // namespace internal

/// @brief Visit variable with strong type.
/// @param func Callable with signature func(var, T{})
template <typename F, typename T>
auto visit(F &&func, T &&var) -> decltype(auto) {
    return std::visit(
        [args = FWD_CAPTURE(func, var)](auto t) {
            auto &&[func, var] = args;
            return std::invoke(FWD(func), FWD(var), t);
        },
        internal::dispatch_as_variant(var.getType().getTypeClass()));
}

template <typename Att, typename Buffer,
          REQUIRES_(std::is_base_of<netCDF::NcAtt, Att>)>
void getattr(const Att &att, Buffer &buf) {
    auto name = att.getName();
    auto type = att.getType();
    constexpr auto buftype_class =
        internal::nctype_v<typename Buffer::value_type>;
    if (buftype_class != type.getTypeClass()) {
        throw std::runtime_error(fmt::format(
            "cannot get attr {} of type {} to buffer of type {}", name,
            type.getName(), netCDF::NcType{buftype_class}.getName()));
    }
    if constexpr (std::is_same_v<Buffer, std::string>) {
        att.getValues(buf);
    } else {
        // check length
        auto alen = att.getAttLength();
        auto blen = buf.size();
        if (alen == blen) {
            att.getValues(buf.data());
        } else {
            throw std::runtime_error(
                fmt::format("cannot get attr {} of len {} to buffer of len {}",
                            name, alen, blen));
        }
    }
}

template <typename T, typename Att,
          REQUIRES_(std::is_base_of<netCDF::NcAtt, Att>)>
T getattr(const Att &att) {
    using namespace netCDF;
    constexpr bool is_scalar = std::is_arithmetic_v<T>;
    using Buffer = std::conditional_t<is_scalar, std::vector<T>, T>;
    auto len = att.getAttLength();
    // check scalar
    if constexpr (is_scalar) {
        if (len > 1) {
            throw std::runtime_error(
                fmt::format("cannot get attr {} of len {} as a scalar",
                            att.getName(), len));
        }
    }
    // get var
    Buffer buf;
    buf.resize(len);
    getattr(att, buf);
    if constexpr (is_scalar) {
        return buf[0];
    } else {
        return buf;
    }
}

template <typename var_t_>
struct pprint {
    using var_t = var_t_;
    const var_t &nc;
    pprint(const var_t &nc_) : nc(nc_) {}

    static auto format_ncvaratt(const netCDF::NcVarAtt &att) {
        std::stringstream os;
        os << fmt::format("{}", att.getName());
        visit(
            [&os](const auto &att, auto t) {
                using T = DECAY(t);
                // handle char with string
                if constexpr (std::is_same_v<T, char>) {
                    auto buf = getattr<std::string>(att);
                    std::size_t maxlen = 70;
                    os << ": \"" << buf.substr(0, maxlen)
                       << (buf.size() > maxlen ? " ..." : "")
                       << fmt::format("\" ({})", att.getType().getName());
                    return;
                } else {
                    auto buf = getattr<std::vector<T>>(att);
                    os << " "
                       << fmt::format("{} ({})", buf, att.getType().getName());
                    return;
                }
            },
            att);
        return os.str();
    }

    static auto format_ncvar(const netCDF::NcVar &var,
                             std::size_t key_width = 0) {
        std::stringstream os;
        std::string namefmt{"{}"};
        if (key_width > 0) {
            namefmt = fmt::format("{{:>{}s}}", key_width);
        }
        os << fmt::format(" {}: ({})", fmt::format(namefmt, var.getName()),
                          var.getType().getName());
        auto dims = var.getDims();
        if (dims.size() > 0) {
            os << " [";
            for (auto it = dims.begin(); it != dims.end(); ++it) {
                if (it != dims.begin())
                    os << ", ";
                os << fmt::format("{}({})", it->getName(), it->getSize());
            }
            os << "]";
        }
        const auto &atts = var.getAtts();
        if (atts.size() > 0) {
            // os << " {";
            for (const auto &att : var.getAtts()) {
                os << fmt::format("\n {} {}", std::string(key_width + 1, ' '),
                                  format_ncvaratt(att.second));
            }
            // os << "}";
        }
        return os.str();
    }

    static auto format_ncfile(const netCDF::NcFile &fo) {
        std::stringstream os;
        // print out some info
        os << "info:\nsummary:\n"
           << fmt::format("    n_vars: {}\n", fo.getVarCount())
           << fmt::format("    n_atts: {}\n", fo.getAttCount())
           << fmt::format("    n_dims: {}\n", fo.getDimCount())
           << fmt::format("    n_grps: {}\n", fo.getGroupCount())
           << fmt::format("    n_typs: {}\n", fo.getTypeCount())
           << "variables:";
        const auto &vars = fo.getVars();
        auto max_key_width = 0;
        std::for_each(
            vars.begin(), vars.end(), [&max_key_width](auto &it) mutable {
                if (auto size = it.first.size(); size > max_key_width) {
                    max_key_width = size;
                }
            });
        for (const auto &var : fo.getVars()) {
            os << "\n" << pprint::format_ncvar(var.second, max_key_width);
        }
        return os.str();
    }

    template <typename OStream>
    friend OStream &operator<<(OStream &os, const pprint &pp) {
        using var_t = typename pprint::var_t;
        if constexpr (std::is_same_v<var_t, netCDF::NcFile>) {
            return os << pprint::format_ncfile(pp.nc);
        } else if constexpr (std::is_same_v<var_t, netCDF::NcVar>) {
            return os << pprint::format_ncvar(pp.nc);
        } else if constexpr (std::is_same_v<var_t, netCDF::NcVarAtt>) {
            return os << pprint::format_ncvaratt(pp.nc);
        } else {
            static_assert(meta::always_false<var_t>::value,
                          "UNABLE TO FORMAT TYPE");
        }
    }
};

} // namespace  nc_utils
