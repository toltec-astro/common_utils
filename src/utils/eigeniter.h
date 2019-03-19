#pragma once
#include <Eigen/Core>
#include <cassert>
#include <fmt/format.h>
#include <iterator>
#include "meta.h"

namespace eigeniter {

/**
 * @brief Class to simulate iterator behavior for Eigen types.
 */
template <typename _Derived> struct EigenIter {
    using Derived = typename Eigen::internal::ref_selector<_Derived>::type;
    using Scalar = typename _Derived::Scalar;

    using value_type = typename std::iterator_traits<Scalar *>::value_type;
    using reference = typename std::iterator_traits<Scalar *>::reference;
    using difference_type =
        typename std::iterator_traits<Scalar *>::difference_type;
    using pointer = typename std::iterator_traits<Scalar *>::pointer;
    using iterator_category = std::random_access_iterator_tag;
    using self = EigenIter;

    EigenIter(const Eigen::DenseBase<_Derived> &m, difference_type n_)
        : data(const_cast<Scalar *>(m.derived().data())), n(n_),
          outer(std::decay<Derived>::type::IsRowMajor ? m.cols() : m.rows()),
          outer_stride(m.outerStride()), inner_stride(m.innerStride()) {}

    self &operator++() {
        ++n;
        return *this;
    }
    self operator++(int) {
        self tmp = *this;
        ++n;
        return tmp;
    }
    self &operator+=(difference_type x) {
        n += x;
        return *this;
    }
    self &operator--() {
        --n;
        return *this;
    }
    self operator--(int) {
        self tmp = *this;
        --n;
        return tmp;
    }
    self &operator-=(difference_type x) {
        n -= x;
        return *this;
    }
    reference operator[](difference_type n) {
        // index to block
        n += this->n;
        auto i = n / outer;
        auto j = n - i * outer;
        // index to data
        return data[i * outer_stride + j * inner_stride];
    }
    reference operator*() { return this->operator[](0); }
    // friend operators
    friend bool operator==(const self &x, const self &y) {
        assert(x.data == y.data);
        return x.n == y.n;
    }
    friend bool operator!=(const self &x, const self &y) {
        assert(x.data == y.data);
        return x.n != y.n;
    }
    friend bool operator<(const self &x, const self &y) {
        assert(x.data == y.data);
        return x.n < y.n;
    }
    friend bool operator>(const self &x, const self &y) { return y < x; }
    friend bool operator>=(const self &x, const self &y) { return !(x < y); }
    friend bool operator<=(const self &x, const self &y) { return !(y < x); }
    friend difference_type operator-(const self &x, const self &y) {
        assert(x.data == y.data);
        return (x.n - y.n);
    }
    friend self operator-(const self &x, difference_type y) {
        assert(x.data == y.data);
        self tmp{x};
        tmp.n -= y;
        return tmp;
    }
    friend self operator+(const self &x, difference_type y) {
        assert(x.data == y.data);
        self tmp{x};
        tmp.n += y;
        return tmp;
    }
    friend self operator+(difference_type x, const self &y) { return y + x; }
    template <typename OStream>
    friend OStream &operator<<(OStream &out, const self &iter) {
        return out << fmt::format(
                   "eigeniter@{:x} n={} outer={} stride=({}, {})",
                   reinterpret_cast<std::uintptr_t>(iter.data), iter.n,
                   iter.outer, iter.outer_stride, iter.inner_stride);
    }
    Scalar *data;
    difference_type n;

protected:
    difference_type outer;
    difference_type outer_stride;
    difference_type inner_stride;
};

/**
 * @brief Returns linear access iterator-sentinel [begin, end) for Eigen types.
 */
template <typename Derived>
decltype(auto) iters(const Eigen::DenseBase<Derived> &m) {
    return std::make_pair(EigenIter(m.derived(), 0),
                          EigenIter(m.derived(), m.size()));
}

/**
 * @brief Returns std::invoke args with iterator-sentinel [begin, end) of Eigen types.
 */
template <typename Derived, typename... Args>
decltype(auto) iter_args(const Eigen::DenseBase<Derived> &m, Args...args) {
    return std::make_tuple(EigenIter(m.derived(), 0),
                           EigenIter(m.derived(), m.size()),
                           FWD(args)...);
}

/**
 * @brief Apply iterator based function on Eigen types.
 * @param m The Eigen container.
 * @param func Callable that takes an iterator-sentinel pair [begin, end).
 */
template <typename Derived, typename Func>
decltype(auto) apply(const Eigen::DenseBase<Derived> &m, Func &&func) {
    auto [begin, end] = iters(m.derived());
    return std::forward<decltype(func)>(func)(begin, end);
}

} // namespace eigeniter