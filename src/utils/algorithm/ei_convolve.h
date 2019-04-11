#pragma once
#include "../meta_enum.h"
#include <unsupported/Eigen/CXX11/Tensor>
#include "../container.h"

namespace alg {

template<typename DerivedA, typename DerivedB>
auto convolve1d(const Eigen::DenseBase<DerivedA>& vector,
                const Eigen::DenseBase<DerivedB>& kernel) {
    auto data = Eigen::TensorMap<const Eigen::Tensor<typename DerivedA::Scalar, 1>>(
        vector.derived().data(), vector.size());
    auto ker = Eigen::TensorMap<const Eigen::Tensor<typename DerivedB::Scalar, 1>>(
        kernel.derived().data(), kernel.size());
    typename DerivedA::PlainObject output = data.convolve(ker, {0});
    return output;
}

template<typename Derived, typename F>
auto convolve1d(const Eigen::DenseBase<Derived>& vector_,
                F&& func, Eigen::Index size) {
    auto& vector = const_cast<Eigen::DenseBase<Derived>&>(vector_).derived();
    auto data = Eigen::TensorMap<Eigen::Tensor<typename Derived::Scalar, 1>>(
        vector.data(), {vector.size()});
    Eigen::Tensor<typename Derived::Scalar, 2> patches_ = data.extract_patches(Eigen::array<std::ptrdiff_t, 1>{size});
    Eigen::Map<const Eigen::Matrix<
        typename Derived::Scalar, Eigen::Dynamic, Eigen::Dynamic>> patches
        (patches_.data(), patches_.dimension(0), patches_.dimension(1));
    auto icols = container_utils::index(patches.cols());
    typename Derived::PlainObject output(patches.cols());
    std::for_each(icols.begin(), icols.end(), [&] (auto i) {
        output.coeffRef(i) = FWD(func)(patches.col(i));
    });
    return output;
}

}  // namespace alg