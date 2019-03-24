#include <benchmark/benchmark.h>
#include <gtest/gtest.h>
#include <iostream>
#include <utils/grppiex.h>

namespace {

TEST(formatter, variant) {
    std::variant<bool, int, double, const char *, std::string> v;
    using namespace std::literals;
    v = false;
    SPDLOG_TRACE("v={}", v);
    v = -1;
    SPDLOG_TRACE("v={}", v);
    v = 2e4;
    SPDLOG_TRACE("v={}", v);
    v = "v";
    SPDLOG_TRACE("v={}", v);
    v = "test"s;
    SPDLOG_TRACE("v={}", v);
}

TEST(formatter, pprint) {
    using fmt_utils::pprint;
    Eigen::MatrixXd m{5, 10};
    Eigen::VectorXd::Map(m.data(), m.size())
        .setLinSpaced(m.size(), 0, m.size());
    SPDLOG_TRACE("default m{}", m);
    SPDLOG_TRACE("m{}", pprint{m});
    SPDLOG_TRACE("m{:r1c5}", pprint{m});
    SPDLOG_TRACE("m{:r1c}", pprint{m});
    SPDLOG_TRACE("m{:rc1}", pprint{m});
    auto c = m.col(0);
    SPDLOG_TRACE("default c{}", c);
    SPDLOG_TRACE("c{:}", pprint{c});
    SPDLOG_TRACE("c{:rc}", pprint{c});
    SPDLOG_TRACE("c{:s}", pprint{c});
    SPDLOG_TRACE("c{:s3}", pprint{c});
    std::vector<double> v = {0, 1, 2, 3, 4, 5, 6, 7};
    SPDLOG_TRACE("default v{:s4}", v);
    SPDLOG_TRACE("v{:}", pprint{v});
    SPDLOG_TRACE("v{:rc}", pprint{v});
    SPDLOG_TRACE("v{:s}", pprint{v});
    SPDLOG_TRACE("v{:s3}", pprint{v});
}

TEST(formatter, pointer) {
    int a = 1;
    SPDLOG_TRACE("a={}", a);
    SPDLOG_TRACE("*a@{}", fmt::ptr(&a));
    SPDLOG_TRACE("*a@{:x}", fmt_utils::ptr(&a));
    SPDLOG_TRACE("*a@{:y}", fmt_utils::ptr(&a));
    SPDLOG_TRACE("*a@{:z}", fmt_utils::ptr(&a));
    EXPECT_NO_THROW(fmt::format("{}", fmt_utils::ptr(&a)));
    EXPECT_NO_THROW(fmt::format("{:y}", fmt_utils::ptr(&a)));
    EXPECT_NO_THROW(fmt::format("{:z}", fmt_utils::ptr(&a)));
    EXPECT_EQ(fmt::format("a@{}", fmt_utils::ptr(&a)),
              fmt::format("a@{:z}", fmt_utils::ptr(&a)));
}
TEST(formatter, eigeniter) {
    Eigen::MatrixXd m(5, 5);
    auto [begin, end] = eigeniter::iters(m);
    SPDLOG_TRACE("iters begin={:s} end={:s}", begin, end);
    SPDLOG_TRACE("begin={:l} end={:l}", begin, end);
    EXPECT_NO_THROW(fmt::format("{}", begin));
    EXPECT_NO_THROW(fmt::format("{:s}", begin));
    EXPECT_EQ(fmt::format("{}", end), fmt::format("{:l}", end));
}

TEST(formatter, bitmask) {
    auto bm = grppiex::Mode::seq | grppiex::Mode::omp;
    SPDLOG_TRACE("bitmask: {}", bm);
    SPDLOG_TRACE("bitmask: {:d}", bm);
    SPDLOG_TRACE("bitmask: {:s}", bm);
    SPDLOG_TRACE("par: {:d}", bitmask::bitmask{grppiex::Mode::par});
    SPDLOG_TRACE("par: {:s}", bitmask::bitmask{grppiex::Mode::par});
    SPDLOG_TRACE("par: {:l}", bitmask::bitmask{grppiex::Mode::par});
    EXPECT_EQ(fmt::format("{}", bm), fmt::format("{:l}", bm));
}

TEST(meta_enum, meta) {
    using meta = grppiex::Mode_meta;
    SPDLOG_TRACE("{}: {}", meta::name, meta::to_name(grppiex::Mode::par));
    SPDLOG_TRACE("{}: {}", meta::name, meta::to_name(grppiex::Mode::omp));
    SPDLOG_TRACE("{}: {}", meta::name,
                 meta::to_name(static_cast<grppiex::Mode>(0)));
    SPDLOG_TRACE("enum: {}", meta::meta);
    SPDLOG_TRACE("par: {:d}", meta::from_name("par"));
    SPDLOG_TRACE("par: {:s}", meta::from_name("par"));
    SPDLOG_TRACE("par: {:l}", meta::from_name("par"));
    SPDLOG_TRACE("par val: {:s}", grppiex::Mode::par);
    SPDLOG_TRACE("unknown: {}", meta::from_name("unknown"));
    EXPECT_NO_THROW(fmt::format("{}", meta::meta));
    EXPECT_NO_THROW(fmt::format("{:s}", meta::from_name("omp")));
    EXPECT_NO_THROW(fmt::format("{}", meta::from_name("xyz")));
}

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
    for (const auto & m : modes) {
        SPDLOG_TRACE("modes: {}", m);
    }
    auto vm = utils::create<std::vector<std::string>>(modes, [](const auto& m){return std::string(m.name);});
    for (const auto & m : vm) {
        SPDLOG_TRACE("vector modes: {}", m);
    }
    fmt::format("{}", vm);
    SPDLOG_TRACE("vector modes: {}", vm);
    std::vector<int> a = {1, 2, 3};
    SPDLOG_TRACE("vec int: {}", a);
    auto b = utils::create<std::vector<double>>(a);
    SPDLOG_TRACE("vec double: {}", b);
    std::string s = "abc";
    SPDLOG_TRACE("string: {}", s);
    auto sm = utils::create<std::set<std::string>>(std::move(vm));
    auto vms = vm.size();
    SPDLOG_TRACE("modes after: {}, {}", vm, vms);
    SPDLOG_TRACE("set modes: {}", sm);
    std::vector<std::string> test = {"abc", "def", "ghi"};
    auto cs = utils::create<std::vector<std::string>>(std::move(test));
    SPDLOG_TRACE("test after: {}", test);
    SPDLOG_TRACE("cs: {}", cs);
    std::copy(std::make_move_iterator(test.begin()),
                   std::make_move_iterator(test.end()),
                   std::inserter(cs, cs.end()));
    SPDLOG_TRACE("test after: {}", test);
    SPDLOG_TRACE("cs: {}", cs);
}

TEST(utils, eigeniter) {
    Eigen::MatrixXd m{5, 10};
    Eigen::VectorXd::Map(m.data(), m.size())
        .setLinSpaced(m.size(), 0, m.size() - 1);
    SPDLOG_TRACE("m{}", m);
    auto [begin, end] = eigeniter::iters(m);
    for (auto it = begin; it != end; ++it) {
        SPDLOG_TRACE("v={}", *it);
    }
}

TEST(utils, meanstd) {
    Eigen::VectorXd m;
    m.setLinSpaced(100, 0, 99);
    SPDLOG_TRACE("m{}", m);
    auto [mean1, std1] = utils::meanstd(m);
    SPDLOG_TRACE("mean={} std={}", mean1, std1);
    Eigen::VectorXI n;
    n.setLinSpaced(10, 1, 10);
    SPDLOG_TRACE("n{}", n);
    auto [mean, std] = utils::meanstd(n);
    auto [med, mad] = utils::medmad(n);
    SPDLOG_TRACE(
            "dmean = {} mean = {} std={}, median={}, mad={}",
            n.mean(), mean, std, med, mad);
}

TEST(utils, fill_linspaced) {
    Eigen::MatrixXd m{5, 10};
    utils::fill_linspaced(m, 0, 98);
    SPDLOG_TRACE("m{}", m);
    utils::fill_linspaced(m.topLeftCorner(2, 2), 0, 98);
    utils::fill_linspaced(m.topRightCorner(2, 2), 0, 98);
    SPDLOG_TRACE("m{:rc}", m);
    utils::fill_linspaced(m.bottomRows(3), 0, 98);
    SPDLOG_TRACE("m{:rc}", m);
    utils::fill_linspaced(m.row(0).head(2), 0, 19);
    SPDLOG_TRACE("m{}", m);
    utils::fill_linspaced(m.col(1).segment(2, 2), 0, 19);
    SPDLOG_TRACE("m{}", m);
    Eigen::VectorXd v{20};
    utils::fill_linspaced(v, 0, 19);
    SPDLOG_TRACE("v{}", v);
    utils::fill_linspaced(v.segment(2, 2), 0, 19);
    SPDLOG_TRACE("v{}", v);

}

TEST(utils, std_eigen) {
    Eigen::MatrixXd m{5, 10};
    utils::fill_linspaced(m, 0, 98);
    SPDLOG_TRACE("m{:r0c0s0}", m);
    SPDLOG_TRACE("m{:r0c0s0}", fmt_utils::pprint(m));
    auto v1 = utils::tostd(m);
    SPDLOG_TRACE("v1{}", v1);
    auto v2 = utils::tostd(m.topRightCorner(2, 2));
    SPDLOG_TRACE("v2{}", v2);
    auto v3 = utils::tostd(m.row(1).segment(1, 2));
    SPDLOG_TRACE("v3{}", v3);
    auto m1 = utils::aseigen(v3);
    SPDLOG_TRACE("m1{}", m1);
    SPDLOG_TRACE("m1*m1{}", m1.array().square());
}

TEST(utis, polyfit) {
    Eigen::MatrixXd m{10, 2};
    utils::fill_linspaced(m, 0, 2);
    SPDLOG_TRACE("m{:s0}", m);
    auto [p1, r1] = utils::polyfit(m.col(0), m.col(1));
    SPDLOG_TRACE("linear fit: p={} r={}", p1, r1);
    Eigen::MatrixXd det;
    auto [p2, r2] = utils::polyfit(m.col(0), m.col(1), 3, &det);
    SPDLOG_TRACE("linear fit: det={}", det);
}

} // namespace

int main(int argc, char *argv[]) {
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
