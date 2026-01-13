#ifndef EZTIMER_HPP
#define EZTIMER_HPP

#include <chrono>
#include <vector>
#include <string>
#include <cstddef>
#include <random>
#include <optional>
#include <functional>
#include <cassert>

/**
 * @file eztimer.hpp
 * @brief Easy timing of C++ functions.
 */

/**
 * @namespace eztimer
 * @brief Easy timing of C++ functions.
 */
namespace eztimer {

/**
 * @brief Options for `time()`.
 */
struct Options {
    /**
     * Maximum number of iterations to run each function.
     * This does not include the `burn_in` iterations.
     */
    int iterations = 10;

    /**
     * Number of burn-in iterations to run each function before timing.
     * Times for these iterations are not reported.
     */
    int burn_in = 1;

    /**
     * Seed for the random number generator, used to randomize the function execution order at each iteration.
     */
    unsigned long long seed = 123456;

    /**
     * Maximum time to run each function, in seconds.
     * Once this is exceeded, all remaining iterations are skipped for that function. 
     *
     * Time for burn-in iterations is not included here.
     *
     * Ignored if not set.
     */
    std::optional<std::chrono::duration<double> > max_time_per_function;

    /**
     * Maximum time to run `time()`, in seconds.
     * Once this is exceeded, all remaining calls of all functions are skipped.
     *
     * Time for burn-in iterations is not included here. 
     *
     * Ignored if not set.
     */
    std::optional<std::chrono::duration<double> > max_time_total;
};

/**
 * @brief Timings for each function.
 */
struct Timings {
    /**
     * Vector of timings for each run of the function, in seconds.
     */
    std::vector<std::chrono::duration<double> > times;

    /**
     * Mean of `times`, in seconds.
     */
    std::chrono::duration<double> mean = std::chrono::duration<double>(0);

    /**
     * standard deviation of `times`, in seconds.
     */
    std::chrono::duration<double> sd = std::chrono::duration<double>(0);
};

/**
 * Record the execution time of any number of functions, possibly over multiple iterations.
 * When multiple functions are supplied, they are called in a random order per iteration to avoid any dependencies.
 * Burn-in iterations are performed at the start to ensure any initialization effects do not distort the timings.
 * The per-function or total runtime can also be capped, in which case the actual number of iterations for a function may be less than `Options::iterations`.
 *
 * @param funs Vector of functions to be timed.
 * Each function should return a value that depends on the computation of interest, to ensure that the latter is not optimized away by the compiler.
 * @param check Function that accepts a `Result_` and an index of `funs`, and performs some kind of check on the former.
 * Any check is fine as long as it uses the value returned by `funs`.
 * The runtime of this function will not be included in the timings.
 * @param opt Further options.
 *
 * @return Vector of length equal to `funs.size()`,
 * containing the timings for each function.
 *
 * @tparam Result_ Result of each function call.
 */
template<typename Result_>
std::vector<Timings> time(
    const std::vector<std::function<Result_()> >& funs,
    const std::function<void(const Result_&, std::size_t)>& check,
    const Options& opt
) {
    const auto nfun = funs.size();
    const int num_iterations = opt.iterations + opt.burn_in;

    // Create a random execution sequence, so no function gets a consistent
    // benefit from running after another function.
    std::mt19937_64 rng(opt.seed);
    std::vector<std::size_t> order;
    order.reserve(num_iterations * nfun);
    for (int i = 0; i < num_iterations; ++i) {
        for (std::size_t f = 0; f < nfun; ++f) {
            order.push_back(f);
        }
        std::shuffle(order.end() - nfun, order.end(), rng);
    }

    std::vector<Timings> output(nfun);
    auto total_time = std::chrono::duration<double>(0);
    auto oIt = order.begin();

    for (int i = 0; i < num_iterations; ++i) {
        for (std::size_t f = 0; f < nfun; ++f, ++oIt) {
            const auto current = *oIt;
            auto& curout = output[current];

            if (i >= opt.burn_in) {
                if (opt.max_time_per_function.has_value() && curout.mean >= *(opt.max_time_per_function)) {
                    continue;
                }
                if (opt.max_time_total.has_value() && total_time >= *(opt.max_time_total)) {
                    continue;
                }
            }

            const auto start = std::chrono::steady_clock::now();
            auto res = funs[current]();
            const auto end = std::chrono::steady_clock::now();
            const auto curtime = std::chrono::duration_cast<std::chrono::duration<double> >(end - start);
            curout.times.push_back(curtime);

            if (i >= opt.burn_in) {
                check(res, current);
                curout.mean += curtime;
                if (opt.max_time_total.has_value()) {
                    total_time += curtime;
                }
            }
        }
    }

    assert(oIt == order.end());
    for (auto& curout : output) {
        // Throwing away the burn-in cycles. We add them and throw them away to
        // ensure that the compiler doesn't just optimize out the calls.
        curout.times.erase(curout.times.begin(), curout.times.begin() + opt.burn_in);
        if (curout.times.empty()) {
            continue;
        }

        // Computing the remaining statistics.
        curout.mean /= curout.times.size();
        for (const auto id : curout.times) {
            const double delta = (id - curout.mean).count();
            curout.sd += std::chrono::duration<double>(delta * delta);
        }
        curout.sd /= curout.times.size() - 1;
        curout.sd = std::chrono::duration<double>(curout.sd.count());
    }

    return output; 
}

}

#endif
