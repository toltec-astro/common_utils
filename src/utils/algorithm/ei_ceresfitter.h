#pragma once

#include <Eigen/Core>
#include <ceres/ceres.h>
#include "../meta.h"
#include "../logging.h"
#include "../eigen.h"
#include "../formatter/matrix.h"

namespace alg {

namespace ceresfit {

using Eigen::Index;
using Eigen::Dynamic;

using ceres::AutoDiffCostFunction;
using ceres::CauchyLoss;
using ceres::CostFunction;
using ceres::Problem;
using ceres::Solver;
using ceres::SubsetParameterization;

/// @brief The mode of the evaluation
/// @var Residual
///     Evaluate residual.
/// @var Model
///     Evaluate model.
enum class EvalMode {Residual, Model};

/*
/// @class that wraps a fitter with model interface
template <typename Fitter>
struct AsModel {
    AsModel(Fitter* fitter_): fitter(fitter_) {}
    template<typename... Args>
    auto operator () (Args ... args) {
        return fitter->eval<EvalMode::Model>(FWD(args)...);
    }
private:
    Fitter* fitter;
};
*/

/// @brief Defines properties for parameter
template<typename Scalar>
struct ParamSetting {
        std::string_view name = "unnammed";
        Scalar value = 0;
        bool fixed = false;
        Scalar fixed_value = std::nan("1");
        bool bounded = false;
        Scalar lower_bound = -std::numeric_limits<Scalar>::infinity();
        Scalar upper_bound = std::numeric_limits<Scalar>::infinity();
};


/// @brief Base class to use to define a model and fit with ceres.
/// @tparam _NP The number of mode parameters
/// @tparam _ND_IN The input dimension (number of independant variables) of the model.
/// @tparam _ND_OUT The Output dimension of the model.
/// @tparam _Scalar The numeric type to use.
template <Index _NP = Dynamic, Index _ND_IN = Dynamic, Index _ND_OUT = Dynamic,
          typename _Scalar=double> struct Fitter {
    constexpr static Index NP = _NP;
    constexpr static Index ND_IN = _ND_IN;
    constexpr static Index ND_OUT = _ND_OUT;
    using Scalar = _Scalar;
    /*
    using InputType = Eigen::Matrix<Scalar, NP, 1>;
    using InputDataType = Eigen::Matrix<Scalar, Dynamic, ND>;
    using InputDataBasisType = Eigen::Matrix<Scalar, Dynamic, 1>;
    using ValueType = Eigen::Matrix<Scalar, Dynamic, 1>;
    */

    using InputType = Eigen::Map<const Eigen::Matrix<Scalar, NP, 1>>;
    using ParamSettings = std::vector<ParamSetting<Scalar>>;
    Fitter() = default;

    /// @brief Create autodiff cost function by providing number of residuals
    template <typename Fitter_>
    static auto set_autodiff_residual (Problem* problem, Scalar* paramblock, Fitter_* fitter) {
        // setup residual block
        CostFunction *cost_function =
            new AutoDiffCostFunction<Fitter_, Dynamic, NP>(
                fitter, fitter->ny);
        // problem->AddResidualBlock(cost_function, nullptr, params);
        problem->AddResidualBlock(cost_function, new CauchyLoss(0.5), paramblock);
    }

    /// @brief Create ceres problem by providing parameters
    template<typename Derived>
    static auto make_problem(
            const Eigen::DenseBase<Derived> & params_,
            ParamSettings settings = {}) {
        auto& params = const_cast<Eigen::DenseBase<Derived>&>(params_).derived();
        if constexpr (eigen_utils::is_plain_v<Derived>) {
            if (auto size = params.size(); size != NP) {
                params.derived().resize(NP);
                // SPDLOG_TRACE("resize params size {} to {}", size, params.size());
            }
        }
        // makesure params.data() is continugous up to NP
        if (params.innerStride() != 1 || params.innerSize() < NP) {
            throw std::runtime_error(fmt::format("fitter requires params data of size {}"
                                     " in continugous memory", NP));
        }

        // default settings make use of params values
        if (settings.size() == 0) {
            settings.resize(NP);
            for (std::size_t i = 0; i < NP; ++i) {
                settings[i].value = params.coeff(i);
            }
        }
        // ensure NP param settings if user supplied
        if (auto size = settings.size(); size != NP) {
            throw std::runtime_error(
                    fmt::format("param setting size {} mismatch params size {}", size, NP));
        }

        // setup problem
        auto problem = std::make_shared<Problem>();
        problem->AddParameterBlock(params.data(), NP);
        std::vector<int> fixed_params;
        // setup params
        for (Index i = 0; i < NP; ++i) {
            auto p = settings[i];
            if (p.bounded) {
                problem->SetParameterLowerBound(params.data(), i,
                                                p.lower_bound);
                problem->SetParameterUpperBound(params.data(), i,
                                                p.upper_bound);
            }
            if (p.fixed) {
                params.coeffRef(i) = p.fixed_value;
                fixed_params.push_back(i);
            } else {
                params.coeffRef(i) = p.value;
            }
        }
        // SPDLOG_TRACE("params init values: {}", params);
        // SPDLOG_TRACE("fixed params: {}", fixed_params);
        if (fixed_params.size() > 0) {
            SubsetParameterization *sp = new SubsetParameterization(
                        NP, fixed_params);
            problem->SetParameterization(params.data(), sp);
        }
        return std::make_pair(problem, params.data());
    }

    /// to be implemented by subclass
    template<EvalMode ev> void eval() {}
};

template <typename Fitter_, typename DerivedA, typename DerivedB, typename DerivedC, typename DerivedD>
bool fit(const Eigen::DenseBase<DerivedA>& xdata_,
         const Eigen::DenseBase<DerivedB>& ydata_,
         const Eigen::DenseBase<DerivedC>& yerr_,
         Eigen::DenseBase<DerivedD> const & params_
         )
{
    Eigen::DenseBase<DerivedD>& params = const_cast<Eigen::DenseBase<DerivedD>&>(params_);
    auto& xdata = xdata_.derived();
    auto& ydata = ydata_.derived();
    auto& yerr = yerr_.derived();
    // make sure the data are continuous
    if (!(eigen_utils::is_contiguous(xdata) &&
          eigen_utils::is_contiguous(ydata) &&
          eigen_utils::is_contiguous(yerr)  &&
          eigen_utils::is_contiguous(params)
          )) {
        throw std::runtime_error("fit does not work with non-continugous data");
    }
    // construct Fitter
    auto fitter = new Fitter_();
    // create problem
    auto [problem, paramblock] = Fitter_::make_problem(params.derived());
    // setup inputs
    // SPDLOG_TRACE("input xdata{}", fmt_utils::pprint(xdata.data(), xdata.size()));
    // SPDLOG_TRACE("input ydata{}", fmt_utils::pprint(reinterpret_cast<const typename Fitter_::Scalar*>(ydata.data()), ydata.size() * 2));
    // SPDLOG_TRACE("input yerr{}", fmt_utils::pprint(reinterpret_cast<const typename Fitter_::Scalar*>(yerr.data()), yerr.size() * 2));

    fitter->nx = xdata.size();
    fitter->ny = ydata.size() * 2;  // complex

    fitter->xdata = xdata.data();
    fitter->ydata = reinterpret_cast<const typename Fitter_::Scalar*>(ydata.data());
    fitter->yerr = reinterpret_cast<const typename Fitter_::Scalar*>(yerr.data());

    // setup residuals
    Fitter_::set_autodiff_residual(
            problem.get(), paramblock, fitter);

    // SPDLOG_TRACE("initial params {}",
    //        fmt_utils::pprint(paramblock, Fitter_::NP));
    // SPDLOG_TRACE("xdata{}", fmt_utils::pprint(fitter->xdata, fitter->nx));
    // SPDLOG_TRACE("ydata{}", fmt_utils::pprint(fitter->ydata, fitter->ny));
    // SPDLOG_TRACE("yerr{}", fmt_utils::pprint(fitter->yerr, fitter->ny));

    // do the fit
    Solver::Options options;
    options.linear_solver_type = ceres::DENSE_QR;
    // options.minimizer_progress_to_stdout = true;
    options.logging_type = ceres::SILENT;
    Solver::Summary summary;
    ceres::Solve(options, problem.get(), &summary);

    SPDLOG_TRACE("{}", summary.BriefReport());
    // SPDLOG_TRACE("fitted paramblock {}",
    //        fmt_utils::pprint(paramblock, Fitter_::NP));
    // SPDLOG_DEBUG("fitted params {}", params.derived());
    return summary.termination_type == ceres::CONVERGENCE;
}



}  // namspace ceresfit
}  // namespace alg
