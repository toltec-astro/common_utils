#include <benchmark/benchmark.h>
#include <gtest/gtest.h>

#include "utils/algorithm/ei_linspaced.h"
#include "utils/algorithm/ei_polyfit.h"
#include "utils/algorithm/ei_stats.h"
#include "utils/formatter/container.h"
#include "utils/formatter/enum.h"
#include "utils/formatter/matrix.h"
#include "utils/grppiex.h"
#include "utils/container.h"
#include <CCfits/CCfits>

namespace {

TEST(grppiex, modes) {
    SPDLOG_TRACE("using default modes: {:s}", grppiex::modes::enabled());
    SPDLOG_TRACE("default mode: {}", grppiex::default_mode());
    EXPECT_EQ(
        grppiex::default_mode_name(grppiex::Mode::seq | grppiex::Mode::omp),
        grppiex::default_mode_name(grppiex::Mode::omp));
    EXPECT_EQ(grppiex::default_mode_name(grppiex::Mode::par),
              grppiex::default_mode_name(grppiex::Mode::omp));
    // custom modes
    using ms = grppiex::Modes<grppiex::Mode::seq, grppiex::Mode::omp>;
    SPDLOG_TRACE("using modes: {:s}", ms::enabled());
    EXPECT_EQ(ms::default_name(grppiex::Mode::seq | grppiex::Mode::omp),
              ms::default_name(grppiex::Mode::seq));
    EXPECT_EQ(ms::default_name(grppiex::Mode::par),
              ms::default_name(grppiex::Mode::omp));
    // dyn_ex
    EXPECT_NO_THROW(grppiex::dyn_ex("omp"));
    EXPECT_NO_THROW(ms::dyn_ex(grppiex::Mode::par));
    EXPECT_NO_THROW(ms::dyn_ex(grppiex::Mode::seq));
}

TEST(utils, create) {
    auto modes = grppiex::Mode_meta::members;
    for (const auto &m : modes) {
        SPDLOG_TRACE("modes: {}", m);
    }
    auto vm = container_utils::create<std::vector<std::string>>(
        modes, [](const auto &m) { return std::string(m.name); });
    for (const auto &m : vm) {
        SPDLOG_TRACE("vector modes: {}", m);
    }
    fmt::format("{}", vm);
    SPDLOG_TRACE("vector modes: {}", vm);
    std::vector<int> a = {1, 2, 3};
    SPDLOG_TRACE("vec int: {}", a);
    auto b = container_utils::create<std::vector<double>>(a);
    SPDLOG_TRACE("vec double: {}", b);
    std::string s = "abc";
    SPDLOG_TRACE("string: {}", s);
    auto sm = container_utils::create<std::set<std::string>>(std::move(vm));
    auto vms = vm.size();
    SPDLOG_TRACE("modes after: {}, {}", vm, vms);
    SPDLOG_TRACE("set modes: {}", sm);
    std::vector<std::string> test = {"abc", "def", "ghi"};
    auto cs = container_utils::create<std::vector<std::string>>(std::move(test));
    SPDLOG_TRACE("test after: {}", test);
    SPDLOG_TRACE("cs: {}", cs);
    std::copy(std::make_move_iterator(test.begin()),
              std::make_move_iterator(test.end()), std::inserter(cs, cs.end()));
    SPDLOG_TRACE("test after: {}", test);
    SPDLOG_TRACE("cs: {}", cs);
}

TEST(alg, meanstd) {
    Eigen::VectorXd m;
    m.setLinSpaced(100, 0, 99);
    SPDLOG_TRACE("m{}", m);
    auto [mean1, std1] = alg::meanstd(m);
    SPDLOG_TRACE("mean={} std={}", mean1, std1);
    Eigen::VectorXI n;
    n.setLinSpaced(10, 1, 10);
    SPDLOG_TRACE("n{}", n);
    auto [mean, std] = alg::meanstd(n);
    auto [med, mad] = alg::medmad(n);
    SPDLOG_TRACE("dmean = {} mean = {} std={}, median={}, mad={}", n.mean(),
                 mean, std, med, mad);
}

TEST(alg, fill_linspaced) {
    Eigen::MatrixXd m{5, 10};
    alg::fill_linspaced(m, 0, 98);
    SPDLOG_TRACE("m{}", m);
    alg::fill_linspaced(m.topLeftCorner(2, 2), 0, 98);
    alg::fill_linspaced(m.topRightCorner(2, 2), 0, 98);
    SPDLOG_TRACE("m{:rc}", m);
    alg::fill_linspaced(m.bottomRows(3), 0, 98);
    SPDLOG_TRACE("m{:rc}", m);
    alg::fill_linspaced(m.row(0).head(2), 0, 19);
    SPDLOG_TRACE("m{}", m);
    alg::fill_linspaced(m.col(1).segment(2, 2), 0, 19);
    SPDLOG_TRACE("m{}", m);
    Eigen::VectorXd v{20};
    alg::fill_linspaced(v, 0, 19);
    SPDLOG_TRACE("v{}", v);
    alg::fill_linspaced(v.segment(2, 2), 0, 19);
    SPDLOG_TRACE("v{}", v);
}

TEST(eigen_utils, std_eigen) {
    Eigen::MatrixXd m{5, 10};
    alg::fill_linspaced(m, 0, 98);
    SPDLOG_TRACE("m{}", m);
    auto v1 = eigen_utils::tostd(m);
    SPDLOG_TRACE("v1{}", v1);
    auto v2 = eigen_utils::tostd(m.topRightCorner(2, 2));
    SPDLOG_TRACE("v2{}", v2);
    auto v3 = eigen_utils::tostd(m.row(1).segment(1, 2));
    SPDLOG_TRACE("v3{}", v3);
    auto m1 = eigen_utils::asvec(v3);
    SPDLOG_TRACE("m1{}", m1);
    SPDLOG_TRACE("m1*m1{}", m1.array().square());
}

TEST(alg, polyfit) {
    Eigen::MatrixXd m{10, 2};
    alg::fill_linspaced(m, 0, 2);
    SPDLOG_TRACE("m{:s0}", m);
    auto [p1, r1] = alg::polyfit(m.col(0), m.col(1));
    SPDLOG_TRACE("linear fit: p={} r={}", p1, r1);
    Eigen::MatrixXd det;
    auto [p2, r2] = alg::polyfit(m.col(0), m.col(1), 3, &det);
    SPDLOG_TRACE("linear fit: det={}", det);
}

TEST(fits_utils, ccfits) {
    CCfits::FITS::setVerboseMode("True");

    // make some example data
    auto [r, c] = std::pair<Eigen::Index, Eigen::Index>{40, 50};
    std::map<std::string, Eigen::MatrixXd> data_items;
    data_items["signal"] = Eigen::MatrixXd::Constant(r, c, 1.);
    data_items["weight"] = Eigen::MatrixXd::Constant(r, c, 2.);
    SPDLOG_TRACE("create fits from data: {}", data_items);

    // declare auto-pointer to FITS at function scope. Ensures no resources
    // leaked if something fails in dynamic allocation.
    std::unique_ptr<CCfits::FITS> pFits(nullptr);
    // note the leading !, this is to overwrite exist fits
    const std::string filepath("!test_fits_file_from_common_utils.fits");
    try
    {
        // create the fits object with empty primary hdu
        // we'll add images later
        pFits.reset( new CCfits::FITS(filepath , CCfits::Write) );
    }
    catch (CCfits::FITS::CantCreate)
    {
          // ... or not, as the case may be.
        SPDLOG_ERROR("unable to create file {}", filepath);
    }
    // add some header to primary hdu
    pFits->pHDU().addKey("OBJECT", "common_utils_fits_utils_ccfits_test", "");
    pFits->pHDU().writeDate();
    for (const auto& [k, v]: data_items) {
        SPDLOG_TRACE("create img ext {} with data {}", k, v);
        // note the order of axis. naxis1 is x which is col
        std::vector naxes{v.cols(), v.rows()};
        auto hdu = pFits->addImage(k, DOUBLE_IMG, naxes);
        // add data
        // the data needs to be wrapped in std::valarray
        std::valarray<double> tmp(v.data(), v.size());
        // write all the data to hdu
        // note that fits is 1-based. so the first pixel has to be 1
        hdu->write(1, tmp.size(), tmp);
        // add wcs to the img hdu
        hdu->addKey("CTYPE1", "RA---TAN", "");
        hdu->addKey("CTYPE2", "DEC--TAN", "");
        // center pixel. note the order of axis. crpix1 is x which is col
        // note the extra 1 in the crpix, because fits is 1-based
        hdu->addKey("CRPIX1", v.cols() / 2. + 1, "");
        hdu->addKey("CRPIX2", v.rows() / 2. + 1, "");
        // coords of the ref pixel in degrees
        hdu->addKey("CUNIT1", "deg", "");
        hdu->addKey("CUNIT2", "deg", "");
        hdu->addKey("CRVAL1", 180., "");
        hdu->addKey("CRVAL2", 60., "");
        // CD matrix. We assume pxiel schale of 1arcsec, and no rota
        double pixsize_arcsec = 1.;
        double pixsize_deg = pixsize_arcsec / 3600.;
        hdu->addKey("CD1_1", -pixsize_deg, "");
        hdu->addKey("CD1_2", 0, "");
        hdu->addKey("CD2_1", 0, "");
        hdu->addKey("CD2_2", pixsize_deg, "");
    }

    // let's check if we got them correct
    SPDLOG_TRACE("Fits Info:\n{}\n", pFits->pHDU());
    for (auto kv: pFits->extension()) {
        SPDLOG_TRACE("extension {}:\n{}", kv.first, *kv.second);
    }
}

} // namespace

int main(int argc, char *argv[]) {
    logging::init<>(true);
    testing::InitGoogleTest(&argc, argv);
    benchmark::Initialize(&argc, argv);
    std::cout << "Running tests:\n";

    int result = RUN_ALL_TESTS();
    if (result == 0) {
        std::cout << "\nRunning benchmarks:\n";
        benchmark::RunSpecifiedBenchmarks();
    }
    return result;
}
