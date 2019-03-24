#pragma once
#include "bitmask.h"
#include "meta.h"
#include "meta_enum.h"
#include "utils.h"
#include <dyn/dynamic_execution.h>
#include <grppi.h>
#include <numeric>

namespace grppiex {

#ifdef DOXYGEN
/**
 * @enum Mode
 * @var seq
 *  Sequential execution.
 * @var thr
 *  Parallel execution with platform native threading.
 * @var omp
 *  Parallel execution with OpenMP.
 * @var tbb
 *  Parallel execution with Intel TBB
 * @var ff
 *  Parallel execution with FastFlow.
 */
enum class Mode : int {
    seq = 1 << 0,
    thr = 1 << 1,
    omp = 1 << 2,
    tbb = 1 << 3,
    ff = 1 << 4
};
#else
meta_enum_class(Mode, int, seq = 1 << 0, thr = 1 << 1, omp = 1 << 2,
                tbb = 1 << 3, ff = 1 << 4, par = thr | omp | tbb | ff);
BITMASK_DEFINE_MAX_ELEMENT(Mode, ff);
#endif

/*
 * @brief Manage the available GRPPI execution modes.
 * @tparam modes The modes to use, sorted from high priority to low.
 * If not set, a default order is used: {omp, thr, tbb, ff, seq}.
 */
template <Mode... modes> struct Modes {
private:
    template <Mode... ms> struct Modes_impl {
    private:
        /// @brief All supported GRPPI execution modes.
        constexpr static auto m_supported = []() {
            using T = std::underlying_type_t<Mode>;
            T m = 0;
            using namespace grppi;
            if constexpr (is_supported<sequential_execution>()) {
                m |= static_cast<T>(Mode::seq);
            }
            if constexpr (is_supported<parallel_execution_native>()) {
                m |= static_cast<T>(Mode::thr);
            }
            if constexpr (is_supported<parallel_execution_omp>()) {
                m |= static_cast<T>(Mode::omp);
            }
            if constexpr (is_supported<parallel_execution_tbb>()) {
                m |= static_cast<T>(Mode::tbb);
            }
            if constexpr (is_supported<parallel_execution_ff>()) {
                m |= static_cast<T>(Mode::ff);
            }
            return static_cast<Mode>(m);
        }();
        template <Mode m> constexpr static auto get_supported_filter() {
            if constexpr (m_supported & m) {
                return std::make_tuple(m);
            } else {
                return std::make_tuple();
            }
        }
        constexpr static auto get_supported() {
            auto s = std::tuple_cat(get_supported_filter<ms>()...);
            static_assert(std::tuple_size<decltype(s)>::value > 0,
                          "NEED TO SPECIFY AT LEAST ONE SUPPORTED MODE");
            return s;
        }

    public:
        constexpr static auto supported = meta::t2a(get_supported());
    };
    using self = std::conditional_t<
        (sizeof...(modes) > 0), Modes_impl<modes...>,
        Modes_impl<Mode::omp, Mode::thr, Mode::tbb, Mode::ff, Mode::seq>>;

public:
    static auto enabled() {
        return std::reduce(self::supported.begin(), self::supported.end(),
                           bitmask::bitmask<Mode>{}, std::bit_or<>{});
    }
    static auto default_(bitmask::bitmask<Mode> mode) {
        for (const auto &m : self::supported) {
            if (m & mode)
                return m;
        }
        throw std::runtime_error(
            fmt::format("unable to get grppi execution mode {:s}", mode));
    }
    /// @brief Returns the default GRPPI execution mode from all supported
    /// modes;
    static auto default_() { return self::supported[0]; }
    /// @brief Returns the default GRPPI execution mode name;
    template <typename... Args> static auto default_name(Args... args) {
        return "";
        // return Mode_meta::to_name(default_(args...));
    }

    /// @brief Returns the GRPPI execution object of \p mode.
    /// Mode with higher prority is used if multiple modes are set.
    template <typename... Args>
    static grppi::dynamic_execution dyn_ex(bitmask::bitmask<Mode> ms,
                                           Args &&... args) {
        using namespace grppi;
        if (!(enabled() & ms))
            throw std::runtime_error(
                fmt::format("grppi execution mode {:s} is not supported", ms));
        return [&](Mode m) -> grppi::dynamic_execution {
            SPDLOG_TRACE("create dynamic execution {}", m);
            switch (m) {
            case Mode::seq: {
                return sequential_execution(FWD(args)...);
            }
            case Mode::thr: {
                return parallel_execution_native(FWD(args)...);
            }
            case Mode::omp: {
                return parallel_execution_omp(FWD(args)...);
            }
            case Mode::tbb: {
                return parallel_execution_tbb(FWD(args)...);
            }
            case Mode::ff: {
                return parallel_execution_ff(FWD(args)...);
            }
            default:
                throw std::runtime_error("unknown grppi execution mode");
            }
        }(default_(ms));
    }
    /// @brief Returns the GRPPI execution object of mode \p name.
    template <typename... Args>
    static grppi::dynamic_execution dyn_ex(std::string_view name,
                                           Args &&... args) {
        auto mode = Mode_meta::from_name(name);
        if (!mode)
            throw std::runtime_error(fmt::format(
                "\"{:s}\" is not a valid grppi execution mode", name));

        return dyn_ex(Mode_meta::from_name(name).value().value, FWD(args)...);
    }
};

/// @brief The default Modes class with all supported modes enabled.
/// @see \ref Modes
using modes = Modes<>;

/// @brief Returns the default GRPPI mode from all supported modes.
/// @see \ref Modes
template <typename... Args> auto default_mode(Args... args) {
    return modes::default_(FWD(args)...);
}

/// @brief Returns the name of the default GRPPI mode.
/// @see \ref default_mode
template <typename... Args> auto default_mode_name(Args... args) {
    return modes::default_name(FWD(args)...);
}
/// @brief Returns the GRPPI execution object of \p mode.
/// @see \ref Modes::dyn_ex
template <typename... Args> auto dyn_ex(Args... args) {
    return modes::dyn_ex(FWD(args)...);
}

} // namespace grppiex