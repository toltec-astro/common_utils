#pragma once
#include "bitmask.h"
#include "meta.h"
#include "meta_enum.h"
#include "utils.h"
#include <dyn/dynamic_execution.h>
#include <grppi.h>

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
/// @brief The default ordering of modes, from high priority to low.
static std::vector<Mode> default_mode_order = {
    Mode::omp, Mode::thr, Mode::tbb, Mode::ff, Mode::seq
};

/// @brief Set the default ordering of modes.
template<typename ... Mode>
void set_default_mode_order (Mode ... modes) {
    default_mode_order = {modes...};
};


/// @brief Returns the supported GRPPI execution modes.
constexpr auto supported_modes() {
    using T = decltype(Mode_meta)::UnderlyingType;
    T mode = 0;
    using namespace grppi;
    if constexpr (is_supported<sequential_execution>()) {
        mode = mode | static_cast<int>(Mode::seq);
    }
    if constexpr (is_supported<parallel_execution_native>()) {
        mode = mode | static_cast<int>(Mode::thr);
    }
    if constexpr (is_supported<parallel_execution_omp>()) {
        mode = mode | static_cast<int>(Mode::omp);
    }
    if constexpr (is_supported<parallel_execution_tbb>()) {
        mode = mode | static_cast<int>(Mode::tbb);
    }
    if constexpr (is_supported<parallel_execution_ff>()) {
        mode = mode | static_cast<int>(Mode::ff);
    }
    return bitmask::bitmask(static_cast<Mode>(mode));
}
/// @brief Returns the number of supported GRPPI execution modes.
constexpr auto supported_mode_count() {
    constexpr auto mode = supported_modes();
    return meta::bitcount(static_cast<int>(supported_modes().bits()));
}

/// @brief Returns the supported GRPPI execution mode names.
constexpr auto supported_mode_names() {
    constexpr auto mode = supported_modes();
    std::array<std::string_view, supported_mode_count()> ret;
    int i = 0;
    for (const auto &m : Mode_meta.members) {
        if (m.value | mode) {
            ret[i] = m.name;
            ++i;
        }
    }
    return ret;
}

/// @brief Returns the default GRPPI execution mode.
/// @param modes The modes to choose from
inline auto default_mode(bitmask::bitmask<Mode> modes) {
    for (const auto& m: default_mode_order) {
        if (m & modes) return m;
    }
    throw std::runtime_error("unable to infer default grppi execution mode");
}

/// @brief Returns the default GRPPI execution mode from all supported modes;
inline auto default_mode() {
    return default_mode(supported_modes());
}

/// @brief Returns the default GRPPI execution mode name;
template <typename ... Args>
auto default_mode_name(Args ... args) {
    return Mode_value_to_string(default_mode(args...));
}

/// @brief Returns the GRPPI execution object of \p mode.
/// \p default_mode_order is used if multiple modes are set.
template <typename... Args>
grppi::dynamic_execution dyn(bitmask::bitmask<Mode> modes, Args &&... args) {
    using namespace grppi;
    if (!(supported_modes() & modes))
        throw std::runtime_error(
            // fmt::format("grppi execution mode {} is not supported",
            //           Mode_value_to_string(modes))
                "modes not supported");
    return [&](Mode mode) -> grppi::dynamic_execution {
        switch (mode) {
            case Mode::seq:
                return sequential_execution(FWD(args)...);
            case Mode::thr:
                return parallel_execution_native(FWD(args)...);
            case Mode::omp:
                return parallel_execution_omp(FWD(args)...);
            case Mode::tbb:
                return parallel_execution_tbb(FWD(args)...);
            case Mode::ff:
                return parallel_execution_ff(FWD(args)...);
            default:
                throw std::runtime_error("unknown grppi execution mode");
        }
    }(default_mode(modes));
}

/// @brief Returns the GRPPI execution object of mode \p name.
template <typename... Args>
grppi::dynamic_execution dyn(std::string_view name, Args &&... args) {
    return dyn(Mode_meta_from_name(name).value().value, FWD(args)...);
}

} // namespace ex