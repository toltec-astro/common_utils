#pragma once

#ifndef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif
#ifndef SPDLOG_FMT_EXTERNAL
#define SPDLOG_FMT_EXTERNAL
#endif
#include <Eigen/Core>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <spdlog/spdlog.h>
#include <variant>

namespace logging {

namespace internal {

template <typename OStream, typename Derived>
OStream &pprint_matrix(OStream &s, const Eigen::DenseBase<Derived> &m_,
                       const Eigen::IOFormat &fmt, int max_rows, int max_cols,
                       int max_size = -1) {
    auto &&m = m_.derived();
    if (m.size() == 0) {
        s << fmt.matPrefix << fmt.matSuffix;
        return s;
    }
    if (max_size < 0)
        max_size = max_rows * max_cols;
    if (m.cols() == 1 || m.rows() == 1) {
        max_rows = max_size;
        max_cols = max_size;
    }

    std::streamsize explicit_precision;
    if (fmt.precision == Eigen::StreamPrecision) {
        explicit_precision = 0;
    } else {
        explicit_precision = fmt.precision;
    }
    std::streamsize old_precision = 0;
    if (explicit_precision)
        old_precision = s.precision(explicit_precision);

    // iterate-and-apply F to m items from head and tail of container with n
    // items in total
    auto partial_apply = [](auto n, auto m, auto &&F) {
        if (n <= m) {
            for (decltype(n) i = 0; i < n; ++i)
                std::forward<decltype(F)>(F)(i);
        } else {
            for (decltype(n) i = 0; i < m / 2; ++i) {
                std::forward<decltype(F)>(F)(i);
            }
            for (decltype(n) i = n - m / 2 - 1; i < n; ++i) {
                std::forward<decltype(F)>(F)(i);
            }
        }
    };
    bool align_cols = !(fmt.flags & Eigen::DontAlignCols);
    Eigen::Index width = 0;
    if (align_cols) {
        partial_apply(m.rows(), max_rows - 1, [&](auto i) {
            partial_apply(m.cols(), max_cols - 1, [&](auto j) {
                std::stringstream sstr;
                sstr.copyfmt(s);
                sstr << m.coeff(i, j);
                width = std::max(width, decltype(i)(sstr.str().length()));
            });
        });
    }
    std::string ellipsis = "...";
    std::string fmt_ellipsis = "...";
    if (width > 0) {
        if (width <= 3) {
            fmt_ellipsis = std::string(width, '.');
        } else {
            fmt_ellipsis = std::string(width, ' ');
            fmt_ellipsis.replace((width - ellipsis.size() + 1) / 2,
                                 ellipsis.size(), ellipsis);
        }
    }
    s << fmt.matPrefix;
    Eigen::Index oi = -1; // use this to detect jump of coeffs, at which point
                          // "..." is inserted
    partial_apply(m.rows(), max_rows - 1, [&](auto i) {
        decltype(i) oj = -1;
        partial_apply(m.cols(), max_cols - 1, [&](auto j) {
            if (j == 0) {
                // if (i > 0) s << fmt.rowSpacer;
                s << fmt.rowPrefix;
            } else {
                s << fmt.coeffSeparator;
            }
            if (j > oj + 1) {
                s << ellipsis;
            } else {
                if (width > 0) {
                    s.width(width);
                }
                if (i > oi + 1) {
                    s << fmt_ellipsis;
                } else {
                    s << m.coeff(i, j);
                }
            }
            if (j == m.cols() - 1) {
                s << fmt.rowSuffix;
                if (i < m.rows() - 1) {
                    s << fmt.rowSeparator;
                }
            }
            oj = j;
        });
        oi = i;
    });
    s << fmt.matSuffix;
    if (explicit_precision) {
        s.precision(old_precision);
    }
    return s;
}

template<typename T>
using is_scalar = std::is_arithmetic<typename std::decay<T>::type>;
} // namespace internal

/**
 * @brief Class to pretty print data as Eigen types.
 */
template <typename T>
struct pprint {
    /// Type to store the data. If T is Eigen type, the underlaying Eigen
    /// refenrence type is used, otherwise it is Eigen::Map
    using Ref =
        typename Eigen::internal::ref_selector<typename std::conditional<
            internal::is_scalar<T>::value,
            Eigen::Map<const Eigen::Matrix<
                typename std::decay<T>::type, Eigen::Dynamic, Eigen::Dynamic>>,
            T>::type>::type;
    // using a = typename T::nothing;
    // using a = typename Ref::nothing;

    /**
     * @brief Pretty print data held by Eigen types.
     */
    template<typename U=T, typename=std::enable_if_t<!internal::is_scalar<U>::value>>
    pprint(const Eigen::DenseBase<T> &m): m(m.derived()) {}
    /**
     * @name Pretty print raw data buffer as Eigen::Map.
     * @{
     */
    /// The size of matrix data \p m is given by \p nrows and \p ncols.
    pprint(const T *const m, Eigen::Index nrows, Eigen::Index ncols)
        : m(Ref(m, nrows, ncols)) {}
    /// The size of vector data \p v is given by \p size.
    pprint(const T *const v, Eigen::Index size) : pprint(v, size, 1) {}
    /** @}*/
    /**
     * @brief Pretty print data held by std vector
     */
    template<typename U=T, typename=std::enable_if_t<internal::is_scalar<U>::value>>
    pprint(const std::vector<T>& v): pprint(v.data(), v.size()) {}

    template <typename OStream>
    friend OStream &operator<<(OStream &os, const pprint &pp) {
        auto &&m = pp.m;
        if (m.size() == 0)
            return os << "(empty)";
        os << "(" << m.rows() << "," << m.cols() << ")";

        auto f = pp.matrix_formatter();
        return internal::pprint_matrix(os, m, f, 5, 5, 10);
    }

private:
    Ref m;
    auto matrix_formatter() const {
        if (m.cols() == 1 || m.rows() == 1) {
            return Eigen::IOFormat(Eigen::StreamPrecision, Eigen::DontAlignCols,
                                   ", ", ", ", "", "", "[", "]");
        }
        if (m.cols() < 3) {
            return Eigen::IOFormat(Eigen::StreamPrecision, Eigen::DontAlignCols,
                                   ", ", " ", "[", "]", "[", "]");
        }
        return Eigen::IOFormat(Eigen::StreamPrecision, 0, ", ", "\n", "[", "]",
                               "[\n", "]\n");
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

namespace std {

template <typename OStream, typename T0, typename... Ts>
OStream &operator<<(OStream &s, variant<T0, Ts...> const &v) {
    visit([&](auto &&arg) { s << arg; }, v);
    return s;
}

} // namespace std
