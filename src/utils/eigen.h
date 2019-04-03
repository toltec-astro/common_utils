#pragma once
#include <Eigen/Core>
#include <type_traits>

namespace Eigen {

using VectorXI = Eigen::Matrix<Eigen::Index, Eigen::Dynamic, 1>;
using MatrixXI = Eigen::Matrix<Eigen::Index, Eigen::Dynamic, Eigen::Dynamic>;
using VectorXb = Eigen::Matrix<bool, Eigen::Dynamic, 1>;
using MatrixXb = Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic>;

} // namespace Eigen

namespace eigen_utils {

namespace internal {
// Eigen CRTP does not work for vector block
template <typename T> struct is_vector_block : std::false_type {};
template <typename VectorType, int Size>
struct is_vector_block<Eigen::VectorBlock<VectorType, Size>> : std::true_type {
};

} // namespace internal

template <typename T>
inline constexpr bool is_vblock_v =
    internal::is_vector_block<std::decay_t<T>>::value;

template <typename T>
inline constexpr bool is_eigen_v =
    std::is_base_of_v<Eigen::EigenBase<std::decay_t<T>>, std::decay_t<T>> ||
    is_vblock_v<std::decay_t<T>>;

template <typename T>
inline constexpr bool is_dense_v =
    std::is_base_of_v<Eigen::DenseBase<std::decay_t<T>>, std::decay_t<T>>;

/// True if type manages its own data, e.g. MatrixXd, etc.
template <typename T>
inline constexpr bool is_plain_v =
    std::is_base_of_v<Eigen::PlainObjectBase<std::decay_t<T>>, std::decay_t<T>>;

template <typename T, typename = void> struct type_traits : std::false_type {};

template <typename T>
struct type_traits<T, std::enable_if_t<is_eigen_v<T>>> : std::true_type {
    using Derived = typename std::decay_t<T>;
    constexpr static Eigen::StorageOptions order =
        Derived::IsRowMajor ? Eigen::RowMajor : Eigen::ColMajor;
    // related types
    using Vector = std::conditional_t<
        order == Eigen::RowMajor,
        Eigen::Matrix<typename Derived::Scalar, 1, Derived::ColsAtCompileTime,
                      Eigen::RowMajor>,
        Eigen::Matrix<typename Derived::Scalar, Derived::RowsAtCompileTime, 1,
                      Eigen::ColMajor>>;
    using Matrix =
        Eigen::Matrix<typename Derived::Scalar, Derived::RowsAtCompileTime,
                      Derived::ColsAtCompileTime, order>;
    using VecMap = Eigen::Map<Vector, Eigen::AlignmentType::Unaligned>;
    using MatMap = Eigen::Map<Matrix, Eigen::AlignmentType::Unaligned>;
};

template <typename Derived>
bool is_contiguous(const Eigen::DenseBase<Derived> &m) {
    return (m.outerStride() == m.innerSize()) && (m.innerStride() == 1);
}

/**
 * @brief Create std::vector from data held by Eigen types. Copies the data.
 * @param m The Eigen type.
 * @tparam order The storage order to follow when copying the data.
 * Default is the same as input.
 */
template <typename Derived,
          Eigen::StorageOptions order = type_traits<Derived>::order>
auto tostd(const Eigen::DenseBase<Derived> &m) {
    using Eigen::Dynamic;
    using Scalar = typename Derived::Scalar;
    std::vector<Scalar> vec(m.size());
    Eigen::Map<Eigen::Matrix<Scalar, Dynamic, Dynamic, order>>(
        vec.data(), m.rows(), m.cols()) = m;
    return vec;
}

/**
 * @brief Create std::vector from data held by Eigen types. Copies the data.
 * @param m The Eigen type.
 * @param order The storage order to follow when copying the data.
 */
template <typename Derived>
auto tostd(const Eigen::DenseBase<Derived> &m, Eigen::StorageOptions order) {
    using Eigen::ColMajor;
    using Eigen::RowMajor;
    if (order & RowMajor) {
        return tostd<Derived, RowMajor>(m.derived());
    }
    return tostd<Derived, ColMajor>(m.derived());
}

/**
 * @brief Create Eigen::Map from std::vector.
 * @tparam order The storage order to follow when mapping the data.
 * Default is Eigen::ColMajor.
 */
template <Eigen::StorageOptions order = Eigen::ColMajor, typename Scalar,
          typename... Rest>
auto asvec(std::vector<Scalar, Rest...> &v) {
    return Eigen::Map<Eigen::Matrix<Scalar, Eigen::Dynamic, 1, order>>(
        v.data(), v.size());
}
/**
 * @brief Create Eigen::Map from std::vector.
 * @tparam order The storage order to follow when mapping the data.
 * Default is Eigen::ColMajor.
 */
template <Eigen::StorageOptions order = Eigen::ColMajor, typename Scalar,
          typename... Rest>
auto asvec(const std::vector<Scalar, Rest...> &v) {
    return Eigen::Map<const Eigen::Matrix<Scalar, Eigen::Dynamic, 1, order>>(
        v.data(), v.size());
}

/**
 * @brief Create Eigen::Map from Eigen Matrix.
 * @tparam order The storage order to follow when mapping the data.
 * Default is Eigen::ColMajor.
 */
template <Eigen::StorageOptions order = Eigen::ColMajor, typename Derived>
auto asvec(const Eigen::DenseBase<Derived> &v_) {
    static_assert(is_plain_v<Derived>,
                  "ASVEC ONLY IMPLEMENTED FOR PLAIN OBJECT");
    auto &v = const_cast<Eigen::DenseBase<Derived> &>(v_).derived();
    if (is_contiguous(v)) {
        using Scalar = typename Derived::Scalar;
        return Eigen::Map<Eigen::Matrix<Scalar, Eigen::Dynamic, 1, order>>(
            v.data(), v.size());
    }
    throw std::runtime_error(
        "not able to create vector view for data in non-contiguous memory");
}

/**
 * @brief Create Eigen::Map from std::vector with shape
 * @tparam order The storage order to follow when mapping the data.
 * Default is Eigen::ColMajor.
 */
template <Eigen::StorageOptions order = Eigen::ColMajor, typename Scalar,
          typename... Rest>
auto asmat(std::vector<Scalar, Rest...> &v, Eigen::Index nrows,
           Eigen::Index ncols) {
    using Eigen::Dynamic;
    assert(nrows * ncols == v.size());
    return Eigen::Map<Eigen::Matrix<Scalar, Dynamic, Dynamic, order>>(
        v.data(), nrows, ncols);
}

} // namespace eigen_utils