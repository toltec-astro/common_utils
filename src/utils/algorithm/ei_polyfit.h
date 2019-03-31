#pragma once

#include <Eigen/Dense>

namespace alg {

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
auto polyfit(const Eigen::DenseBase<DerivedA> &x,
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

}  // namespace alg
