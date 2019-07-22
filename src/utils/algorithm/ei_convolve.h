#pragma once
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

template <typename DerivedA, typename F, typename DerivedB>
void convolve1d(const Eigen::DenseBase<DerivedA> &vector_, F &&func,
                Eigen::Index size, Eigen::DenseBase<DerivedB> const &output_) {
    using Scalar = typename DerivedA::Scalar;
    auto &vector = vector_.derived();
    auto &output = const_cast<Eigen::DenseBase<DerivedB> &>(output_).derived();
    auto data = Eigen::TensorMap<Eigen::Tensor<const Scalar, 1>>(
        vector.data(), {vector.size()});
    Eigen::Tensor<Scalar, 2> patches_ =
        data.extract_patches(Eigen::array<std::ptrdiff_t, 1>{size});
    Eigen::Map<const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>>
        patches(patches_.data(), patches_.dimension(0), patches_.dimension(1));
    if (output.size() == 0) {
        output.resize(patches.cols());
    }
    if (patches.cols() != output.size()) {
        throw std::runtime_error("output data has incorrect dimension");
    }
    auto icols = container_utils::index(patches.cols());
    std::for_each(icols.begin(), icols.end(), [&](auto i) {
        output.coeffRef(i) = FWD(func)(patches.col(i));
    });
}

template <typename A, typename F>
auto convolve1d(A &&vector, F &&func, Eigen::Index size) {
    typename A::PlainObject output;
    convolve1d(FWD(vector), FWD(func), size, output);
    return output;
}

}  // namespace alg