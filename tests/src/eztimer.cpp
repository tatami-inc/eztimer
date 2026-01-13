#include <gtest/gtest.h>

#include "eztimer/eztimer.hpp"

#include <thread>
#include <chrono>

class EztimerTest : public ::testing::Test {
protected:
    inline static std::vector<std::function<int()> > funs;
    inline static std::function<void(const int&, std::size_t)> check;

    static void SetUpTestSuite() {
        funs.emplace_back([]() -> int {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return 0;
        });

        funs.emplace_back([]() -> int {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            return 1;
        });

        funs.emplace_back([]() -> int {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            return 2;
        });

        check = [](const int& x, std::size_t i) -> void {
            if (x != static_cast<int>(i)) {
                throw std::runtime_error("whoops, that shouldn't have happened");
            }
        };
    }
};

TEST_F(EztimerTest, Basic) {
    eztimer::Options opt;
    auto output = eztimer::time<int>(funs, check, opt);

    EXPECT_EQ(output.size(), funs.size());
    for (const auto& curout : output) {
        EXPECT_EQ(curout.times.size(), opt.iterations);
        EXPECT_GT(curout.mean.count(), 0);
        EXPECT_GT(curout.sd.count(), 0);
    }
}

TEST_F(EztimerTest, CapPerFunction) {
    eztimer::Options opt;
    opt.max_time_per_function = std::chrono::milliseconds(45);
    auto output = eztimer::time(funs, check, opt);

    EXPECT_EQ(output.size(), funs.size());

    // Unfortunately the macos-latest GitHub runner seems pretty bad at
    // accurately sleeping for an accurate amount of time, so we just
    // have to be fairly relaxed here. If sleeps were accurate, we could
    // just replace the LE with EQ.
    EXPECT_LE(output[0].times.size(), 5);
    EXPECT_LE(output[1].times.size(), 3);
    EXPECT_LE(output[2].times.size(), 2);

    EXPECT_GE(output[0].times.size(), 1);
    EXPECT_GE(output[1].times.size(), 1);
    EXPECT_GE(output[2].times.size(), 1);
}

TEST_F(EztimerTest, CapTotal) {
    eztimer::Options opt;
    opt.max_time_total = std::chrono::milliseconds(40);
    auto output = eztimer::time(funs, check, opt);

    EXPECT_EQ(output.size(), funs.size());
    std::size_t num_tasks = 0; 
    for (const auto& times : output) {
        EXPECT_LE(times.times.size(), 1);
        num_tasks += times.times.size();
    }

    // Should be >= 2, but again, the macos runner is a bit too relaxed with its timings.
    EXPECT_GE(num_tasks, 1);
}
