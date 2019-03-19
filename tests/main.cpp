#include <benchmark/benchmark.h>
#include <gtest/gtest.h>
#include <utils/ex.h>
#include <iostream>

namespace {

TEST (ex, default_order)  {
    for (const auto& m: grppiex::supported_mode_names()) {
        SPDLOG_DEBUG("supported mode: {}", m);
    }
    for (const auto& m: grppiex::default_mode_order) {
        SPDLOG_DEBUG("default mode order: {}", m);
    }
    SPDLOG_DEBUG("default mode: {}", grppiex::default_mode());
    EXPECT_EQ(grppiex::default_mode_name(grppiex::Mode::seq | grppiex::Mode::omp),
              grppiex::default_mode_name(grppiex::Mode::omp));
    EXPECT_EQ(grppiex::default_mode_name(grppiex::Mode::par),
              grppiex::default_mode_name(grppiex::Mode::omp));
    // grppiex::set_default_mode_order(grppiex::Mode::seq, grppiex::Mode::omp);
    // EXPECT_EQ(grppiex::default_mode_name(grppiex::Mode::seq | grppiex::Mode::omp),
    //          grppiex::default_mode_name(grppiex::Mode::seq));
    // EXPECT_EQ(grppiex::default_mode_name(grppiex::Mode::par),
    //        grppiex::default_mode_name(grppiex::Mode::omp));

}

}  // namespace

int main(int argc, char* argv[]) {
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

