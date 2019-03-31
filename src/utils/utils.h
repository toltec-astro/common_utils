#pragma once
#include "logging.h"
#include "meta.h"
#include "eigeniter.h"
#include "eigen.h"

namespace utils {

/// @brief Populate data using data in existing container.
/// Optioanally, when \p func is provided, the elements are mapped using
/// \p func.
template <typename In, typename Out, typename... F,
          typename = std::enable_if_t<meta::is_iterable<In>::value>,
          typename = std::enable_if_t<
              meta::is_sized<Out>::value ||
              meta::is_iterator<Out>::value
              >
          >
void populate(In &&in, Out& out, F &&... func) {
    if constexpr (
                meta::is_sized<Out>::value
                ) {
        if constexpr (
                meta::has_resize<Out, decltype(in.size())>::value) {
            // resize if available
            out.resize(in.size());
        }
        // check size match
        assert(out.size() == in.size());
    }
    // figure out output for transform
    auto out_iter = [&]() {
        if constexpr (eigen_utils::is_eigen_v<Out> && eigen_utils::type_traits<Out>::value) {
            // eigen type
            return eigeniter::EigenIter(out, 0);
        } else if constexpr (meta::is_iterator<Out>::value) {
            return out;
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
    // handle l- and r- values differently for input
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
}

/// @brief Create data from data in existing container. Makes use
/// of \ref populate.
template <typename Out, typename In, typename... F,
          typename = std::enable_if_t<meta::is_iterable<In>::value>,
          typename = std::enable_if_t<std::is_default_constructible_v<Out>>
          >
Out create(In &&in, F &&... func) {
    Out out;
    if constexpr (meta::is_instance<Out, std::vector>::value) {
        // reserve for vector
        // SPDLOG_TRACE("reserve vector{}", in.size());
        out.reserve(in.size());
    }
    if constexpr (eigen_utils::is_plain_v<Out>) {
        SPDLOG_TRACE("create eigen");
        populate(FWD(in), out, FWD(func)...);
        return out;
    } else if constexpr (meta::is_iterable<Out>::value) {
        SPDLOG_TRACE("create stl");
        // populate with iterator
        auto out_iter = [&]() {
            if constexpr (meta::has_push_back<Out>::value) {
                SPDLOG_TRACE("use back_inserter");
                return std::back_inserter(out);
            } else if constexpr (meta::has_insert<Out>::value) {
                SPDLOG_TRACE("use inserter");
                return std::inserter(out, out.end());
            } else {
                static_assert(meta::always_false<Out>::value,
                              "NOT ABLE TO GET OUTPUT ITERATOR");
            }
        }();
        populate(FWD(in), out_iter, FWD(func)...);
        return out;
    } else {
        static_assert(meta::always_false<Out>::value,
                      "NOT KNOW HOW TO POPULATE OUT TYPE"
                      );
    }
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

/// @brief Returns the index of element in vector.
template<typename T>
std::optional<typename std::vector<T>::difference_type> indexof(const std::vector<T>& vec, const T& v) {
    auto it = std::find(vec.begin(), vec.end(), v);
    if (it == vec.end()) {
        return std::nullopt;
    }
    return std::distance(vec.begin(), it);
}

} // namespace utils
