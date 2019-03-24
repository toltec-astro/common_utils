#pragma once
#include "eigeniter.h"
#include "logging.h"
#include "meta.h"
#include <Eigen/Dense>
#include <fmt/format.h>

namespace Eigen {
using VectorXI = Eigen::Matrix<Eigen::Index, Eigen::Dynamic, 1>;
using MatrixXI = Eigen::Matrix<Eigen::Index, Eigen::Dynamic, Eigen::Dynamic>;
using VectorXb = Eigen::Matrix<bool, Eigen::Dynamic, 1>;
using MatrixXb = Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic>;
} // namespace Eigen

namespace utils {

/// @brief Create std container from data in existing std container.
/// Optioanally, when \p func is provided, the elements are mapped using
/// \p func.
/// \p Out could be any std container type or a lambda function that
/// returns an output iterator
template <typename Out, typename In, typename... F,
          typename = std::enable_if_t<meta::is_iterable<In>::value>,
          typename = std::enable_if_t<meta::is_iterable<Out>::value>>
Out create(In &&in, F &&... func) {
    Out out;
    if constexpr (meta::is_instance<Out, std::vector>::value) {
        // reserve for vector
        // SPDLOG_TRACE("reserve vector{}", in.size());
        out.reserve(in.size());
    }
    // output
    auto out_iter = [&]() {
        if constexpr (meta::has_push_back<Out>::value) {
            // SPDLOG_TRACE("use back_inserter");
            return std::back_inserter(out);
        } else if constexpr (meta::has_insert<Out>::value) {
            // SPDLOG_TRACE("use inserter");
            return std::inserter(out, out.end());
        } else if constexpr (std::is_invocable_v<Out>) {
            // SPDLOG_TRACE("use lambda")
            return out();
        } else {
            static_assert(meta::always_false<Out>::value,
                          "NOT ABLE TO GET OUTPUT ITERATOR");
        }
    }();
    // to handle custom transform function
    auto run_with_iter = [&](const auto &begin, const auto &end) {
        // no func provided, run a copy with implicit conversion
        if constexpr ((sizeof...(F)) == 0) {
            // SPDLOG_TRACE("use implicit conversion");
            std::copy(begin, end, out_iter);
            // run transform with func
        } else {
            std::transform(begin, end, out_iter, FWD(func)...);
        };
    };
    // handle l- and r- values differently
    if constexpr (std::is_lvalue_reference_v<In>) {
        // run with normal iter
        SPDLOG_TRACE("use copy iterator");
        run_with_iter(in.begin(), in.end());
    } else {
        // run with move iter
        SPDLOG_TRACE("use move iterator");
        run_with_iter(std::make_move_iterator(in.begin()),
                      std::make_move_iterator(in.end()));
    }
    return out;
}

/// @brief Returns true if \p value ends with \p ending.
inline bool endswith(std::string const &value, std::string const &ending) {
    if (ending.size() > value.size()) {
        return false;
    }
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

/// @brief Rearrange nested vector to a flat one *in place*.
template <typename T> void ravel(std::vector<std::vector<T>> &v) {
    std::size_t total_size = 0;
    for (const auto &sub : v)
        total_size += sub.size(); // I wish there was a transform_accumulate
    std::vector<T> result;
    result.reserve(total_size);
    for (const auto &sub : v)
        result.insert(result.end(), std::make_move_iterator(sub.begin()),
                      std::make_move_iterator(sub.end()));
    v = std::move(result);
}

/// @brief Default index type used by Eigen.
using Eigen::Index;

/**
 * @brief Locates and removes duplicated values in *sorted* vector.
 * @return A vector of n_unique bin edge indexes (hence n_unique + 1 items in
 * total). THe i-th bin edge [i, i + 1) stores the indexes [si, ei) which
 * identifies the i-th unique element in the original vector.
 * @note The input vector is not resized, therefore elements starting at index
 * n_unique are in undetermined state.
 */
template <typename Derived> auto uniquefy(const Eigen::DenseBase<Derived> &m) {
    auto size = m.size();
    std::vector<Index> uindex;
    uindex.push_back(0);
    auto old = m(0);
    for (Index i = 0; i < size; ++i) {
        if (m(i) != old) {
            // encountered a new unique value
            old = m(i); // update old
            const_cast<Eigen::DenseBase<Derived> &>(m)(uindex.size()) =
                old;             // store the unqiue value to upfront
            uindex.push_back(i); // store the unique index
        }
    }
    uindex.push_back(
        size); // this extra allows easier loop through the uindex with [i, i+1)
    return uindex;
}

/**
 * @brief Generate a list of index chunks from given index range.
 * @param start The begin index.
 * @param end The past-last index.
 * @param nchunks Number of chunks to be generated
 * @param overlap The overlap between consecutive chunks.
 */
inline auto indexchunks(Index start, Index end, Index nchunks,
                        Index overlap = 0) {
    std::vector<std::pair<Index, Index>> chunks;
    chunks.resize(nchunks);
    auto size = end - start;
    // handle overlap by working with a streched-out size
    auto s_size = size + overlap * (nchunks - 1);
    auto s_chunk_size = s_size / nchunks; // this is the min chunk size
    auto s_leftover =
        s_size % nchunks; // this will be distributed among the first few chunks
    SPDLOG_TRACE("chunk s_size={} s_chunk_size={} s_leftover={}", s_size,
                 s_chunk_size, s_leftover);
    std::generate(chunks.begin(), chunks.end(),
                  [&, curr = start, i = 0]() mutable {
                      auto step = s_chunk_size + (i < s_leftover ? 1 : 0);
                      // SPDLOG_TRACE("gen with curr={} i={} step={}", curr, i,
                      // step);
                      auto chunk = std::make_pair(curr, curr + step);
                      ++i;
                      curr += step - overlap;
                      // SPDLOG_TRACE("done with curr={} i={} step={}", curr, i,
                      // step);
                      return chunk;
                  });
    return chunks;
}

// true if typename Derived manages its own data, e.g. MatrixXd, etc.
// false if typename Derived is an expression.
template <typename Derived>
struct has_storage
    : std::is_base_of<Eigen::PlainObjectBase<std::decay_t<Derived>>,
                      std::decay_t<Derived>> {};

/**
 * @brief Create std::vector from data held by Eigen types. Copies the data.
 * @param m The Eigen type.
 * @param order The storage order to follow when copying the data.
 * Default is Eigen::ColMajor.
 */
template <typename Derived>
auto tostd(const Eigen::DenseBase<Derived> &m,
           Eigen::StorageOptions order = Eigen::ColMajor) {
    std::vector<typename Derived::Scalar> ret(m.size());
    if (order & Eigen::RowMajor) {
        Eigen::Map<Eigen::Matrix<typename Derived::Scalar, Eigen::Dynamic,
                                 Eigen::Dynamic, Eigen::RowMajor>>(
            ret.data(), m.rows(), m.cols()) = m;
    } else {
        Eigen::Map<Eigen::Matrix<typename Derived::Scalar, Eigen::Dynamic,
                                 Eigen::Dynamic, Eigen::ColMajor>>(
            ret.data(), m.rows(), m.cols()) = m;
    }
    return ret;
}

/**
 * @brief Create Eigen::Map from std::vector.
 */
template <typename Scalar, typename... Rest>
auto aseigen(std::vector<Scalar, Rest...> &v) {
    return Eigen::Map<Eigen::Matrix<Scalar, Eigen::Dynamic, 1>>(v.data(),
                                                                v.size());
}

/**
 * @brief Create Eigen::Matrix from std container with transform applied
 */
template <typename Derived, typename In, typename... Args>
void fill_with_std(const Eigen::DenseBase<Derived> &out, In &&in,
                   Args &&... args) {
    if constexpr (has_storage<Derived>::value) {
        out.derived().resize(in.size());
    } else {
        assert(out.size() == in.size());
    }
    auto iter = [&]() { return eigeniter::EigenIter(out.derived(), 0); };
    create<decltype(iter)>(FWD(in), FWD(args)...);
}

template <typename Derived, typename... Args>
void fill_linspaced(const Eigen::DenseBase<Derived> &m_, Args... args) {
    auto &m = const_cast<Eigen::DenseBase<Derived> &>(m_).derived();
    if constexpr (Derived::IsVectorAtCompileTime == 1) {
        SPDLOG_TRACE("fill vector");
        m.setLinSpaced(m.size(), args...);
    } else if constexpr (has_storage<Derived>::value) {
        if (m.outerStride() > m.innerSize()) {
            throw std::runtime_error(fmt::format(
                "cannot fill data in non-contiguous memory: {:r0c0} outer={} "
                "inner={} outerstride={} innerstride={}",
                m, m.outerSize(), m.innerSize(), m.outerStride(),
                m.innerStride()));
        }
        SPDLOG_TRACE("fill matrix");
        // make a map and assign to it
        Eigen::Map<
            Eigen::Matrix<typename Derived::Scalar, Derived::RowsAtCompileTime,
                          1, Derived::Options>,
            Eigen::AlignmentType::Unaligned>(m.data(), m.size())
            .setLinSpaced(m.size(), args...);
    } else {
        using Matrix =
            Eigen::Matrix<typename Derived::Scalar, Derived::RowsAtCompileTime,
                          Derived::ColsAtCompileTime,
                          Derived::IsRowMajor ? Eigen::RowMajor
                                              : Eigen::ColMajor>;
        SPDLOG_TRACE("fill expr");
        Matrix mat{m.rows(), m.cols()};
        fill_linspaced(mat, args...);
        m = std::move(mat);
    }
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
    auto v = tostd(m);
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

/**
 * @brief Fit polynomial to 2-D data vectors.
 * @param x The x values.
 * @param y The y values.
 * @param order The order. Default is 1 (linear fit)
 * @param det If specified, it will hold the transformation matrix
 * @return A tuple of two elements:
 *  - 0: The polynomial coefficients [x0, x1, ...] such that xi is for x^i.
 *  - 1: The residual vector. Same size as x and y.
 *
 */
template <typename DerivedA, typename DerivedB>
decltype(auto) polyfit(const Eigen::DenseBase<DerivedA> &x,
                       const Eigen::DenseBase<DerivedB> &y, int order = 1,
                       Eigen::MatrixXd *det = nullptr) {
    auto s = x.size();
    Eigen::MatrixXd det_;
    if (!det) {
        det = &det_;
    }
    det->resize(s, order + 1);
    for (auto i = 0; i < order + 1; ++i) {
        det->col(i) = x.derived().array().pow(i);
    }
    Eigen::VectorXd pol = det->colPivHouseholderQr().solve(y.derived());
    Eigen::VectorXd res = y.derived() - (*det) * pol;
    return std::make_tuple(std::move(pol), std::move(res));
}

/**
 * @brief Algorithm to find elements from iterative thresholding.
 * @param statsfunc Function that computes [mean, dev] pair of a series.
 * @param selectfunc Function that returns true for elements to be kept. It
 * takes [elem, center, std] as the input, and return.
 * @param max_iter The max number of iterations for iterative clipping.
 * @return A callable that returns a vector of indexes for input data such that
 * data[i] satisfies \p select function.
 */
template <typename F1, typename F2>
decltype(auto) iterclip(F1 &&statsfunc, F2 &&selectfunc, int max_iter = 10) {
    return [max_iter = max_iter,
            fargs = FWD_CAPTURE(statsfunc, selectfunc)](const auto &data) {
        auto &&[statsfunc, selectfunc] = fargs;
        // copy the data
        auto clipped = tostd(data);
        double center = 0;
        double std = 0;
        bool converged = false;
        bool selectingcenter =
            false; // check with selectfunc to determin the clip direction
        for (int i = 0; i < max_iter; ++i) {
            auto old_size = clipped.size();
            std::tie(center, std) = FWD(statsfunc)(aseigen(clipped));
            if (FWD(selectfunc)(center, center, std)) {
                // if center is which the selecting range,
                // the clip will remove outside the selection range (selectfunc
                // is keepfunc) otherwise the clip will remove the selection
                // range (selectfunc is clipfunc)
                selectingcenter = true;
            }
            // check each element in clipped to erase or keep
            clipped.erase(std::remove_if(
                              clipped.begin(), clipped.end(),
                              [&center, &std, &selectingcenter,
                               fargs = FWD_CAPTURE(selectfunc)](const auto &v) {
                                  auto &&[selectfunc] = fargs;
                                  auto select = FWD(selectfunc)(v, center, std);
                                  if (selectingcenter) {
                                      // clip outside of selected
                                      return !select;
                                  }
                                  // clip selected
                                  return select;
                              }),
                          clipped.end());
            if (clipped.size() == old_size) {
                // SPDLOG_TRACE("clipping coverged after {} iterations", i + 1);
                converged = true;
                break; // converged
            }
        }
        // reaches max_iter
        if (!converged) {
            SPDLOG_DEBUG("clip fails to converge after {} iterations",
                         max_iter);
        }
        // auto [center, std] = std::forward<decltype(stats)>(stats)(data);
        // SPDLOG_TRACE("indexofthresh m={} s={}", center, std);
        // get the original selected indexes
        std::vector<Index> selectindex;
        auto [begin, end] = eigeniter::iters(data);
        for (auto it = begin; it != end; ++it) {
            auto select = FWD(selectfunc)(*it, center, std);
            // SPDLOG_TRACE("select {} v={}, m={} s={}", s, *it, center, std);
            if (select) {
                selectindex.push_back(it.n);
            }
        }
        return std::make_tuple(selectindex, converged, center, std);
    };
}

} // namespace utils
