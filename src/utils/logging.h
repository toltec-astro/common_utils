#pragma once
#ifndef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif
#ifndef SPDLOG_FMT_EXTERNAL
#define SPDLOG_FMT_EXTERNAL
#endif

#include "formatter/bitmask.h"
#include "formatter/common.h"
#include "formatter/eigeniter.h"
#include "formatter/matrix.h"
#include <fmt/ostream.h>
#include <spdlog/spdlog.h>
#include <variant>

namespace logging {

/**
 * @brief Call a function and log the timing.
 * @param msg The header of the log message.
 * @param func The function to be called.
 * @param params The arguments to call the function with.
 */
template <typename F, typename... Params>
decltype(auto) timeit(std::string_view msg, F &&func, Params &&... params) {
    SPDLOG_INFO("**timeit** {}", msg);
    // get time before function invocation
    const auto &start = std::chrono::high_resolution_clock::now();
    auto report_time = [&msg, &start]() {
        // get time after function invocation
        const auto &stop = std::chrono::high_resolution_clock::now();
        auto elapsed =
            std::chrono::duration_cast<std::chrono::duration<double>>(stop -
                                                                      start);
        SPDLOG_INFO("**timeit** {} finished in {}ms", msg,
                    elapsed.count() * 1e3);
    };
    if constexpr (std::is_same<decltype(func(
                                   std::forward<decltype(params)>(params)...)),
                               void>::value) {
        // static_assert (is_return_void, "return void") ;
        func(std::forward<decltype(params)>(params)...);
        report_time();
    } else {
        // static_assert (!is_return_void, "return something else");
        decltype(auto) ret = std::forward<decltype(func)>(func)(
            std::forward<decltype(params)>(params)...);
        report_time();
        return ret;
    }
};

} // namespace logging
