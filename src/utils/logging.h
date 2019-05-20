#pragma once
#ifndef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif
#ifndef SPDLOG_FMT_EXTERNAL
#define SPDLOG_FMT_EXTERNAL
#endif
#include <fmt/ostream.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "meta.h"

/// Macros to install scope-local logger and log
#define LOGGER_INIT2(level_, name) inline static const auto logger = [] () { \
    auto color_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>(); \
    auto logger = std::make_shared<spdlog::logger>(name, std::move(color_sink)); \
    logger->set_level(level_); \
    return logger; \
}()
#define LOGGER_INIT1(level_) LOGGER_INIT2(level_, "")
#define LOGGER_INIT0() inline static const auto logger = spdlog::default_logger();
#define LOGGER_INIT(...) GET_MACRO(LOGGER_INIT, __VA_ARGS__)
#define LOGGER_TRACE(...) SPDLOG_LOGGER_TRACE(logger, __VA_ARGS__)
#define LOGGER_DEBUG(...) SPDLOG_LOGGER_DEBUG(logger, __VA_ARGS__)
#define LOGGER_INFO(...) SPDLOG_LOGGER_INFO(logger, __VA_ARGS__)
#define LOGGER_WARN(...) SPDLOG_LOGGER_WARN(logger, __VA_ARGS__)
#define LOGGER_ERROR(...) SPDLOG_LOGGER_ERROR(logger, __VA_ARGS__)
#define LOGGER_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(logger, __VA_ARGS__)

namespace logging {

template<typename F, typename...Args>
decltype(auto) quiet(F &&func, Args &&... args) {
    auto default_log_level = spdlog::default_logger()->level();
    spdlog::set_level(spdlog::level::off);
    auto reset = [&default_log_level]() {
        spdlog::set_level(default_log_level);
    };
    if constexpr (std::is_void_v<std::invoke_result_t<F, Args...>>) {
        FWD(func)(FWD(args)...);
        reset();
    } else {
        decltype(auto) ret = FWD(func)(FWD(args)...);
        reset();
        return ret;
    }
};


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
    if constexpr (std::is_void_v<std::invoke_result_t<F, Params...>>) {
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

