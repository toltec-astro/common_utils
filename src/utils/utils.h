#pragma once
#include "eigeniter.h"
#include "logging.h"
#include "meta.h"
#include <Eigen/Dense>
#include <fmt/format.h>
#include <string>
#include <vector>

namespace utils {

/// @brief Convert compile-time array of string_view to vector of string.
template <std::size_t size>
decltype(auto) view2vec(const std::array<std::string_view, size> &arr) {
    std::vector<std::string> ret;
    for (const auto &a : arr) {
        ret.emplace_back(a);
    }
    return ret;
}

/// @brief Returns true if \p value ends with \p ending.
inline decltype(auto) endswith(std::string const &value,
                               std::string const &ending) {
    if (ending.size() > value.size())
        return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

/**
 * @brief Locates and removes duplicated values in *sorted* vector.
 * @return A vector of n_unique bin edge indexes (hence n_unique + 1 items in
 * total). THe i-th bin edge [i, i + 1) stores the indexes [si, ei) which
 * identifies the i-th unique element in the original vector.
 * @note The input vector is not resized, therefore elements starting at index
 * n_unique are in undetermined state.
 */
template <typename Derived>
decltype(auto) uniquefy(const Eigen::DenseBase<Derived> &m) {
    auto size = m.size();
    std::vector<decltype(size)> uindex;
    uindex.push_back(0);
    for (auto [i, old] = std::tuple{0, m(0)}; i < size; ++i) {
        if (m(i) != old) {
            // encountered a new unique value
            old = m(i); // update old
            const_cast<Eigen::DenseBase<Derived> &>(m)(uindex.size()) =
                old;             // store the unqiue value to upfront
            uindex.push_back(i); // store the unique index
        }
    }
    uindex.push_back(size); // this allows easier loop through the uindex
    return uindex;
}

/// @brief Rearrange nested vector to a flat one.
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

/**
 * @brief Generate a list of index chunks from given index range.
 * @param start The begin index.
 * @param end The past-the-end index.
 * @param nchunks Number of chunks to be generated
 * @param overlap The overlap between consecutive chunks.
 */
decltype(auto) generate_chunks(
        Eigen::Index start,
        Eigen::Index end,
        Eigen::Index nchunks,
        Eigen::Index overlap = 0) {
    std::vector<std::pair<Eigen::Index, Eigen::Index>> chunks;
    chunks.resize(nchunks);
    auto size = end - start;
    // handle overlap by working with a streched-out size
    auto s_size = size + overlap * (nchunks - 1);
    auto s_chunk_size = s_size / nchunks; // this is the min chunk size
    auto s_leftover =
        s_size % nchunks; // this will be distributed across the first chunks
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

/**
 * @brief Return mean and stddev.
 * @param m The vector for which the mean and stddev are calculated.
 * @param ddof Delta degree of freedom. See numpy.std.
 */
template <typename Derived>
decltype(auto) meanstd(const Eigen::DenseBase<Derived> &m, int ddof = 0) {
    typename Derived::Scalar mean = m.mean();
    typename Derived::Scalar std = std::sqrt(
        (m.derived().array() - mean).square().sum() / (m.size() - ddof));
    return std::make_pair(mean, std);
}

/**
 * @brief Create std::vector from data held by Eigen types. Copies the data.
 * @param m The Eigen type.
 * @param order The order to follow when copying the data.
 * Default is Eigen::ColMajor.
 */
template <typename Derived>
decltype(auto) eimat2vec(const Eigen::DenseBase<Derived> &m,
                      Eigen::StorageOptions order = Eigen::ColMajor) {
    auto size = m.size();
    std::vector<typename Derived::Scalar> ret(size);
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
template<typename Scalar>
decltype(auto) vec2eimap(std::vector<Scalar>& v) {
    return Eigen::Map<Eigen::Matrix<Scalar, Eigen::Dynamic, 1>>(v.data(), v.size());
}

/**
 * @brief Create Eigen::Matrix from std::vector with transform applied
 */
template <typename T, typename Derived, typename F>
void vec2eimat(const T& c, const Eigen::DenseBase<Derived> &m,
               F&& func) {
    m.derived().resize(c.size());
    eigeniter::apply(m.derived(), [&c, &m, fargs=FWD_CAPTURE(func)](const auto & begin, const auto& end){
        auto&& [func] = fargs;
        std::transform(c.begin(), c.end(), begin, FWD(func));
    });
}

template <typename Derived> auto median(const Eigen::DenseBase<Derived> &m) {
    // copy to a std vector for sort
    auto v = eimat2vec(m);
    auto n = v.size() / 2;
    std::nth_element(v.begin(), v.begin() + n, v.end());
    if (v.size() % 2) {
        return v[n];
    }
    // even sized vector -> average the two middle values
    auto max_it = std::max_element(v.begin(), v.begin() + n);
    return (*max_it + v[n]) / 2.0;
}

template <typename Derived>
decltype(auto) medianmad(const Eigen::DenseBase<Derived> &m) {
    auto med = median(m);
    return std::make_pair(med, median((m.derived().array() - med).abs()));
}

template <typename DerivedA, typename DerivedB>
decltype(auto) polyfit(const Eigen::DenseBase<DerivedA> &x,
                       const Eigen::DenseBase<DerivedB> &y, int order = 1) {
    auto s = x.size();
    Eigen::MatrixXd det(s, order + 1);
    for (auto i = 0; i < order + 1; ++i) {
        det.col(i) = x.derived().array().pow(i);
    }
    Eigen::VectorXd pol = det.colPivHouseholderQr().solve(y.derived());
    Eigen::VectorXd res = y.derived() - det * pol;
    return std::make_tuple(std::move(pol), std::move(res));
}

/**
 * @brief Algorithm to find elements from thresholding.
 * @param statsfunc Function that computes [center, std] pair of a series.
 * @param selectfunc Function that defines the elements to keep, taking [elem,
 * center, std] as the input.
 * @param max_iter The max number of iterations for iterative clipping.
 * @return A callable that returns a vector of indexes for input data such that
 * data[i] satisfies \p select function.
 */
template <typename F1, typename F2>
decltype(auto) iterclip(F1 &&statsfunc, F2 &&selectfunc, int max_iter = 10) {
    return [max_iter = max_iter,
            fargs = FWD_CAPTURE2(statsfunc, selectfunc)](const auto &data) {
        auto &&[statsfunc, selectfunc] = fargs;
        // run an iterative clip
        auto clipped = vector(data);
        double center = 0;
        double std = 0;
        bool converged = false;
        bool selectingcenter =
            false; // check with selectfun to determin the clip direction
        for (int i = 0; i < max_iter; ++i) {
            auto size = clipped.size();
            Eigen::Map<Eigen::VectorXd> tmp(clipped.data(), size);
            std::tie(center, std) = FWD(statsfunc)(tmp);
            if (FWD(selectfunc)(center, center, std)) {
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
                                      return !select;
                                  }
                                  return select;
                              }),
                          clipped.end());
            if (clipped.size() == size) {
                // SPDLOG_TRACE("clipping coverged after {} iterations", i + 1);
                converged = true;
                break; // converged
            }
        }
        if (!converged) {
            SPDLOG_DEBUG("clip fails to converge after {} iterations",
                         max_iter);
        }
        // auto [center, std] = std::forward<decltype(stats)>(stats)(data);
        // SPDLOG_TRACE("indexofthresh m={} s={}", center, std);
        std::vector<int> ret;
        auto [begin, end] = eigeniter::iters(data);
        for (auto it = begin; it != end; ++it) {
            auto select = FWD(selectfunc)(*it, center, std);
            // SPDLOG_TRACE("select {} v={}, m={} s={}", s, *it, center, std);
            if (select) {
                ret.push_back(it.n);
            }
        }
        return std::make_tuple(ret, converged, center, std);
    };
}

// Helper classes to allow logging intermediate results
struct StateCache {};

} // namespace utils
