#pragma once

#include "../eigen.h"
#include <type_traits>

namespace alg {

auto shape = [](const auto &m) {
    return std::make_pair(m.rows(), m.cols());
};

auto span = [](const auto &v) {
    return v.coeff(v.size() - 1) - v.coeff(0);
};

auto step = [](const auto &v, Eigen::Index i = 0) { return v.coeff(i + 1) - v.coeff(i); };


/**
 * @brief Return argmin.
 */
template <typename Derived, typename = std::enable_if_t<
                                std::is_arithmetic_v<typename Derived::Scalar>>>
auto argmin(const Eigen::DenseBase<Derived> &m) {
    static_assert(Derived::IsVectorAtCompileTime, "REQUIRES VECTOR TYPE");
    Eigen::Index imin = 0;
    m.minCoeff(&imin);
    return imin;
}

/**
 * @brief Return arg nearest.
 */
template <typename Derived, typename = std::enable_if_t<
                                std::is_arithmetic_v<typename Derived::Scalar>>>
auto argeq(const Eigen::DenseBase<Derived> &m, typename Derived::Scalar v) {
    static_assert(Derived::IsVectorAtCompileTime, "REQUIRES VECTOR TYPE");
    auto imin = argmin((m.derived().array() - v).abs());
    return std::make_pair(imin, m[imin] - v);
}

/**
 * @brief Return mean.
 * This promote the type to double for shorter types.
 */
template <typename Derived, typename = std::enable_if_t<
                                std::is_arithmetic_v<typename Derived::Scalar>>>
auto mean(const Eigen::DenseBase<Derived> &m) {
    // promote size to Double for better precision
    auto size = static_cast<double>(m.size());
    return m.sum() / size;
}

/**
 * @brief Return mean and stddev.
 * @param m The vector for which the mean and stddev are calculated.
 * @param ddof Delta degree of freedom. See numpy.std.
 * @note If \p m is an expression, it is evaluated twice.
 */
template <typename Derived, typename = std::enable_if_t<
                                std::is_arithmetic_v<typename Derived::Scalar>>>
auto meanstd(const Eigen::DenseBase<Derived> &m, int ddof = 0) {
    auto mean_ = mean(m.derived());
    auto size = static_cast<double>(m.size());
    auto std =
        std::sqrt((m.derived().array().template cast<decltype(mean_)>() - mean_)
                      .square()
                      .sum() /
                  (size - ddof));
    return std::make_pair(mean_, std);
}

/**
 * @brief Return median
 * @note Promotes to double.
 */
template <typename Derived, typename = std::enable_if_t<
                                std::is_arithmetic_v<typename Derived::Scalar>>>
auto median(const Eigen::DenseBase<Derived> &m) {
    // copy to a std vector for sort
    auto v = eigen_utils::tostd(m);
    auto n = v.size() / 2;
    std::nth_element(v.begin(), v.begin() + n, v.end());
    if (v.size() % 2) {
        return v[n] * 1.0; // promote to double
    }
    // even sized vector -> average the two middle values
    auto max_it = std::max_element(v.begin(), v.begin() + n);
    return (*max_it + v[n]) / 2.0;
}

/**
 * @brief Return median absolute deviation
 */
template <typename Derived> auto medmad(const Eigen::DenseBase<Derived> &m) {
    auto med = median(m);
    return std::make_pair(
        med,
        median(
            (m.derived().array().template cast<decltype(med)>() - med).abs()));
}

}  // namespace alg
