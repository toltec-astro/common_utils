#pragma once
#include "../logging.h"
#include "../eigen.h"

namespace alg {

template <typename Derived, typename... Args>
void fill_linspaced(const Eigen::DenseBase<Derived> &m_, Args... args) {
    using Eigen::Index;
    using traits = typename eigen_utils::type_traits<Derived>;
    auto &m = const_cast<Eigen::DenseBase<Derived> &>(m_).derived();
    if constexpr (Derived::IsVectorAtCompileTime) {
        SPDLOG_TRACE("fill vector");
        m.setLinSpaced(m.size(), args...);
    } else if constexpr (traits::is_plain) {
        // check continugous
        assert(m.outerStride() == m.innerSize());
        SPDLOG_TRACE("fill matrix");
        // make a map and assign to it
        traits::VecMap(m.data(), m.size()).setLinSpaced(m.size, args ...);
    } else {
        SPDLOG_TRACE("fill matrix expr");
        typename Derived::PlainObject tmp{m.rows(), m.cols()};
        fill_linspaced(tmp, args...);
        m = std::move(mat);
    }
}

}  // namespace alg
