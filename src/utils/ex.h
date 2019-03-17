#pragma once
#include "bitmask.h"
#include "meta.h"
#include "meta_enum.h"
#include "utils.h"
#include <dyn/dynamic_execution.h>
#include <grppi.h>

namespace ex {

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
                tbb = 1 << 3, ff = 1 << 4);
BITMASK_DEFINE_MAX_ELEMENT(Mode, ff);
#endif
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
    return static_cast<Mode>(mode);
}
/// @brief Returns the supported GRPPI execution mode names.
constexpr decltype(auto) supported_mode_names() {
    constexpr auto mode = supported_modes();
    std::array<std::string_view, meta::bitcount(static_cast<int>(mode))> ret;
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
constexpr auto default_mode() {
    constexpr auto mode = supported_modes();
    if constexpr (mode & Mode::omp)
        return Mode::omp;
    if constexpr (mode & Mode::thr)
        return Mode::thr;
    if constexpr (mode & Mode::tbb)
        return Mode::tbb;
    if constexpr (mode & Mode::ff)
        return Mode::ff;
    if constexpr (mode & Mode::seq)
        return Mode::seq;
    return mode;
}
/// @brief Returns the default GRPPI execution mode name.
constexpr decltype(auto) default_mode_name() {
    return Mode_value_to_string(default_mode());
}
/// @brief Returns the GRPPI execution object of \p mode.
template <typename... Args>
grppi::dynamic_execution dyn(Mode mode, Args &&... args) {
    using namespace grppi;
    if (!(supported_modes() & mode))
        throw std::runtime_error(
            fmt::format("grppi execution mode {} is not supported",
                        Mode_value_to_string(mode)));
    if (mode & Mode::seq)
        return sequential_execution(FWD(args)...);
    if (mode & Mode::thr)
        return parallel_execution_native(FWD(args)...);
    if (mode & Mode::omp)
        return parallel_execution_omp(FWD(args)...);
    if (mode & Mode::tbb)
        return parallel_execution_tbb(FWD(args)...);
    if (mode & Mode::ff)
        return parallel_execution_ff(FWD(args)...);
    return dynamic_execution();
}
/// @brief Returns the GRPPI execution object of mode \p name.
template <typename... Args>
grppi::dynamic_execution dyn(std::string_view name, Args &&... args) {
    return dyn(Mode_meta_from_name(name).value().value, FWD(args)...);
}

} // namespace ex