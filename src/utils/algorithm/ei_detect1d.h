#pragma once
#include "../grppiex.h"
#include "../logging.h"
#include "../meta.h"
#include "../utils.h"
#include <set>

namespace alg {

namespace detect1d {

/**
 * @brief Class to store intermediate results of \ref divconqfinder.
 * This is optionally populated, and passed as the sole argument to
 * \p statecachefunc functor from the \ref divconqfinder call if the
 * functor is user-supplied.
 * @see divconqfinder
 */
template<typename F1, typename F2, typename F3, typename F4,
         typename R1, typename R2, typename R3, typename R4
         >
struct DivConqFinderStateCache {
    Eigen::VectorXd xvec;  // x data vector
    Eigen::VectorXd yvec;  // y data vector
    /// Result of \p chunkfunc.
    R1 chunks;
    /// Results of \p findfunc per chunk.
    std::vector<R2> finds;
    /// Result after aggregating the finds.
    R1 features;
    // Results of \p selectfunc per feature.
    std::vector<R3> selects;
    // Results of \p propfunc per feature.
    std::vector<R4> props;
    // Results of \ref dcfinder call.
    std::vector<typename R4::value_type> results;
};

/**
 * @brief A divide-and-conquer feature finding algorithm.
 *  The returned functor takes a pair of xdata and ydata, produces
 *  a subset of xdata such that the corresponding ydata represent certain
 *  feature.
 * @param chunkfunc The functor that divides the input data into chunks.
 *  Expected signature: vector<pair<Index, Index>>(Index).
 *  - Params: size of input vector.
 *  - Return: Vector of pairs (si, ei) such that each defines a
 *      data chunk data.segment(si, ei-si).
 * @param findfunc The functor to identify feature indexes in each chunk.
 *  Expected signature: optional<tuple<vector<Index>, ...>>(Data, Data).
 *  - Params: segment of xdata and ydata for one chunk
 *  - Return: Optional tuple. The first item of the tuple is a vector of
 * indexes at which data are identified as feature. Additional useful
 * objects may be returned along with it.
 * @param selectfunc The functor to filter the found features.
 *  Expected signature: optional<tuple<...>>(Data, Data).
 *  - Params: segment of xdata and ydata for one found feature.
 *  - Return: Optional tuple that has value if the feature is to be kept.
 *      Additional useful objects may be returned in the tuple.
 * @param propfunc The functor to compute the feature properties.
 *  Expected signature: optional<tuple<PropScalar, ...>>.
 *  - Params: segment of xdata and ydata for one selected feature.
 *  - Return: Optional tuple. The tuple contains properties computed
 *  for the input feature data. The first item of the properties should
 *  be of scalar type such that an Eigen vector could be construced.
 * @param execution The execution mode to use when run.
 * @param statecachefunc If user specified, a DivConqFinderStateCache object
 * is populated with the intermediate results from various fuctor calls. It
 * then gets called at the end of the algorithm run.
 *  Expected signature: void(DivConqFinderStateCache).
 * @return Functor that perform the feature finding.
 *  Expected signature: Eigen::Vector<PropScalar>(Data, Data)
 *  - Params: xdata and ydata that is sorted, and mappable (contiguous in memory) to Eigen::Vector.
 */
template <typename F1, typename F2, typename F3, typename F4, typename F5 = meta::nop,
          // set up compile type constraits for the functors
          typename R1 = REQUIRES_RT(
             meta::rt_is_instance<std::vector, std::pair, F1, Eigen::Index>
             ),
          typename R2 = REQUIRES_RT(
             meta::rt_is_instance<
                  std::optional, std::tuple,
                  F2, Eigen::VectorXd, Eigen::VectorXd
                  >
              ),
          typename R3 = REQUIRES_RT(
             meta::rt_is_instance<
                  std::optional, std::tuple,
                  F3, Eigen::VectorXd, Eigen::VectorXd
                  >
             ),
          typename R4 = REQUIRES_RT(
             meta::rt_is_instance<
                  std::optional, std::tuple,
                  F4, Eigen::VectorXd, Eigen::VectorXd
                  >
              )
          >
auto
divconqfinder(F1 &&chunkfunc, F2 &&findfunc, F3 &&selectfunc, F4 &&propfunc,
         grppi::dynamic_execution execution = grppiex::dyn_ex(),
         F5&& statecachefunc=meta::nop()
        ) {
    return [fargs = FWD_CAPTURE(
                chunkfunc,
                findfunc,
                selectfunc,
                propfunc,
                statecachefunc
            ),
            execution = std::move(execution)](const auto &xdata, const auto &ydata) {
        // unpack the functors
        auto&& [chunkfunc, findfunc, selectfunc, propfunc, statecachefunc] = fargs;
        // create empty cache object
        auto cache = DivConqFinderStateCache<
                F1, F2, F3, F4,
                R1, R2, R3, R4>();
        // true if the statecachefunc is user-supplied
        constexpr bool caching = !meta::is_nop_v<std::decay_t<decltype(statecachefunc)>>;
        // validate data
        auto validate_contiguous_data = [] (const auto & m) {
            // contiguous
            if (m.outerStride() > m.innerSize()) {
                throw std::runtime_error(fmt::format(
                    "input data has to be contiguous in memory: {:r0c0} outer={} "
                    "inner={} outerstride={} innerstride={}",
                    m, m.outerSize(), m.innerSize(), m.outerStride(),
                    m.innerStride()));
            }
        };
        auto validate_sorted = [] (const auto & m) {
            if (!eigeniter::iterapply(m, [](auto&& ... args){
                    return std::is_sorted(args...);
                })) {
                throw std::runtime_error("input data has to be sorted");
            }
        };
        assert(validate_contiguous_data(xdata));
        assert(validate_contiguous_data(ydata));
        assert(validate_sorted(xdata));
        // map xdata and ydata to vectors
        auto size = xdata.size();
        using Eigen::Index;
        using Eigen::Dynamic;

        using XType = typename std::decay_t<decltype(xdata)>;
        using XScalar = typename XType::Scalar;
        constexpr auto XStorage = XType::IsRowMajor?Eigen::RowMajor: Eigen::ColMajor;
        Eigen::Map<const Eigen::Matrix<XScalar, Dynamic, 1, XStorage>> xvec(xdata.data(), size);

        using YType = typename std::decay_t<decltype(ydata)>;
        using YScalar = typename YType::Scalar;
        constexpr auto YStorage = YType::IsRowMajor?Eigen::RowMajor: Eigen::ColMajor;
        Eigen::Map<const Eigen::Matrix<YScalar, Dynamic, 1, YStorage>> yvec(ydata.data(), size);

        // algorithm starts here
        // create chunks
        auto chunks = FWD(chunkfunc)(size);
        // update cache
        if constexpr (caching) {
            SPDLOG_TRACE("cache: xvec {}", xvec);
            SPDLOG_TRACE("cache: yvec {}", yvec);
            SPDLOG_TRACE("cache: chunk n={}", chunks.size());
            cache.xvec = xvec;
            cache.yvec = yvec;
            cache.chunks = chunks;
            cache.finds.resize(chunks.size());
        }
        // find features
        auto featureindex = grppi::map_reduce(
            execution,
            // for each chunk, run findfunc and aggregate the result to a set.
            chunks.begin(), chunks.end(), std::set<Index>{},
            [&xvec, &yvec, &chunks, &cache, fargs=FWD_CAPTURE(findfunc)](
                const auto &chunk) -> std::vector<Index> {
                auto &&[findfunc] = fargs;
                auto csize = chunk.second - chunk.first;
                // SPDLOG_TRACE("working on chunk [{}, {}) size={}",
                // chunk.first, chunk.second, csize);
                // the result is in an optional tuple
                auto found = FWD(findfunc)(
                            xvec.segment(chunk.first, csize),
                            yvec.segment(chunk.first, csize)
                            );
                // shortcut to return empty if nothing found
                if (!found.has_value()) {
                    return {};
                }
                // get the feature index from the returned
                auto& fi = std::get<0>(found.value());
                SPDLOG_TRACE(
                    "feature of length {} found in segment #{} [{}, {}) size={}",
                    fi.size(),
                    utils::indexof(chunks, chunk).value(),
                    chunk.first, chunk.second, csize);
                // restore the correct index w.r.t. the original data
                for (auto &i : fi) {
                    i += chunk.first;
                }
                // update cache
                if constexpr (caching) {
                    cache.finds[utils::indexof(chunks, chunk).value()] = found;
                }
                return fi;
                /*
                if constexpr(plot) {
                    // expect that the findfunc returns [feature, x, y1, y2 ...]
                    std::scoped_lock<std::mutex> lock(plotmutex);
                    // plot the feature
                    auto n = fi.size();
                    std::vector<double> fxvec(n);
                    std::vector<double> fyvec(n);
                    for (Index j = 0; j < n; ++j) {
                        fxvec[j] = xvec(fi[j]);
                        fyvec[j] = xvec(fi[j]);
                    }
                    plotfunc("find", fxvec, fyvec, 0);
                    // plot the extra info
                    if constexpr (meta::tuplesize(tmp) > 2) {
                        auto &x = std::get<1>(tmp);
                        meta::static_for<std::size_t, 2, meta::tuplesize(tmp)>(
                            [&](auto i) {
                                auto &y = std::get<i>(tmp);
                                plotfunc("find", fxvec, fyvec, i - 1);
                            });
                    }
                }
                */
            },
            // reduction op to merge the results to a set
            [](auto &&lhs, auto &&rhs) {
                lhs.insert(std::make_move_iterator(rhs.begin()),
                           std::make_move_iterator(rhs.end()));
                return lhs;
            });
        // further merge the peak of consecutive index to one
        std::vector<std::pair<Index, Index>> features;
        for (const auto &fi : featureindex) {
            if (features.empty() || features.back().second < fi) {
                features.emplace_back(fi, fi + 1); // [begin, past-last] range
            } else {
                features.back().second++;
            }
        }
        SPDLOG_DEBUG("found {} feature candidates", features.size());
        // update cache
        if constexpr (caching) {
            cache.features = features;
            cache.selects.resize(features.size());
            cache.props.resize(features.size());
        }
        // grppi call for each feature
        // store the props of all features
        using prop_t = R4;  // return type of propfunc
        std::vector<prop_t> props(features.size());
        grppi::map(
            execution,
            // for each feature, run propfunc and aggregate the result to a set.
            features.begin(), features.end(),
            props.begin(),
            [&xvec, &yvec, &features, &cache,
             fargs = FWD_CAPTURE(selectfunc, propfunc)](
                const auto &feature) -> prop_t {
                auto &&[selectfunc, propfunc] = fargs;
                auto fi = utils::indexof(features, feature).value();
                SPDLOG_TRACE("checking feature #{}", fi);
                auto fsize = feature.second - feature.first;
                auto fx = xvec.segment(feature.first, fsize);
                auto fy = yvec.segment(feature.first, fsize);
                // apply good value selection
                auto selected = FWD(selectfunc)(fx, fy);
                // shortcut to return if rejected by selectfunc
                if (!selected.has_value()) {
                    SPDLOG_TRACE("feature #{} rejected by selectfunc", fi);
                    return {};
                }
                // update cache
                if constexpr(caching) {
                    cache.selects[fi] = selected;
                }
                // compute property.
                auto prop = FWD(propfunc)(fx, fy);
                // update cache
                if constexpr(caching) {
                    cache.props[fi] = prop;
                }
                if (!prop.has_value()) {
                    SPDLOG_TRACE("feature #{} rejected by propfunc", fi);
                }
                return prop;
        });
        /*
        Eigen::VectorXd ret(props.size());
        eigeniter::apply(ret, [&props](const auto &begin, const auto &end) {
            std::transform(props.begin(), props.end(), begin,
                           [](const auto &v) { return v; });
        });
        */
        // unpack the optionals and return a vector of props
        std::vector<typename prop_t::value_type> results;
        for (auto& prop: props) {
            if (prop.has_value()) {
                results.push_back(prop.value());
            }
        }
        SPDLOG_TRACE("number of detected features: {}", props.size());
        SPDLOG_TRACE("number of results: {}", results.size());
        /*
        ret.reserve(props.size());
        while (!props.empty()) {
            ret.emplace_back(std::move(props.extract(props.begin()).value()));
        }
        */
        // update cache, and apply datacachefunc
        if constexpr(caching) {
            cache.results = results;
            // call the statecachefunc
            FWD(statecachefunc)(cache);
        }
        return results;
    };
}

} //namespace detect1d
}  // namespace finder