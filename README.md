# Easy timing of C++ functions

![Unit tests](https://github.com/tatami-inc/eztimer/actions/workflows/run-tests.yaml/badge.svg)
![Documentation](https://github.com/tatami-inc/eztimer/actions/workflows/doxygenate.yaml/badge.svg)
[![codecov](https://codecov.io/gh/tatami-inc/eztimer/branch/master/graph/badge.svg?token=7I3UBJLHSO)](https://codecov.io/gh/tatami-inc/eztimer)

## Overview

The **eztimer** library implements a simple timing framework for C++ functions,
mostly intended for testing algorithms in other [**tatami**](https://github.com/tatami-inc) libraries.
Users can supply multiple functions and **eztimer** will run them over multiple iterations in randomized order.
The first "burn-in" iteration is also discarded to avoid any initialization effects. 
For convenience, we support both a per-function and overall cap on the runtime.

## Quick start

```cpp
#include "eztimer/eztimer.hpp"

// Setting up functions to be timed.
std::vector<std::function<double()> > funs;
funs.emplace_back([]() -> double {
    double blah = 0;
    // Do something that modifies 'blah' as a side-effect.
    return blah;
});

funs.emplace_back([]() -> double {
    double foo = 0;
    // Do something that modifies 'foo' as a side-effect.
    return foo;
});

funs.emplace_back([]() -> double {
    double bar = 0;
    // Do something that modifies 'bar' as a side-effect.
    return bar;
});

eztimer::Options opt;
opt.iterations = 5;
opt.max_time_per_function = std::chrono::duration<double>(10); // max 10 seconds per function.
opt.max_time_total = std::chrono::duration<double>(60); // max 60 seconds total runtime.

auto output = eztimer::time<double>(
    funs, 
    /* check = */ [](const double& x, std::size_t i) -> void {
        if (x == 0) {
            throw std::runtime_error("expected result of function " + std::to_string(i) + " to be modified"); 
        }
    },
    opt
);

output[0].mean; // mean runtime, in seconds.
output[0].sd; // standard deviation of the runtime across iterations.
output[0].individual; // times for the individual runs.
```

Check out the [reference documentation](https://tatami-inc.github.io/eztimer/) for more details.

## Comments

The `check` argument to `eztimer::time()` ensures that the result of each function call has a user-visible effect on the program.
This prevents the compiler from optimizing away the function call entirely. 
In the example above, we threw an error if the result of each call did not follow our non-zero expectation.
We could also be more concrete if we expected all functions to return the same result:

```cpp
std::optional<double> expected;

auto check = [&](const double& x, std::size_t i) -> void {
    if (expected.has_value()) {
        if (*expected != x) {
            throw std::runtime_error("unexpected result from function " + std::to_string(i)); 
        }
    } else {
        expected = x;
    }
};
```

Another option is to simply print the result to standard output. 
This may also be generally useful to track the progress of `time()` for long-running functions.

```cpp
std::vector<std::string> names { "blah", "foo", "bar" };
auto check = [&](const double& x, std::size_t i) -> void {
    std::cout << "Function '" << names[i] < "' returning " << x << std::endl;
};
```

As for the timing itself, we just measure the wall time taken to execute each function.
We don't bother with the `sys`/`user` distinction, or try to measure cycles, or anything particularly complicated.
It seems pointless to try to be too smart here as execution time depends on so many factors -
if you want generalizable conclusions about performance, the best approach is to repeat the timings on different machines.

## Building projects

### CMake with `FetchContent`

If you're using CMake, you just need to add something like this to your `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(
  eztimer
  GIT_REPOSITORY https://github.com/tatami-inc/eztimer
  GIT_TAG master # or any version of interest
)

FetchContent_MakeAvailable(eztimer)
```

Then you can link to **tatami** to make the headers available during compilation:

```cmake
# For executables:
target_link_libraries(myexe eztimer)

# For libaries
target_link_libraries(mylib INTERFACE eztimer)
```

### CMake using `find_package()`

You can install the library by cloning a suitable version of this repository and running the following commands:

```sh
mkdir build && cd build
cmake .. -DEZTIMER_TESTS=OFF
cmake --build . --target install
```

Then you can use `find_package()` as usual:

```cmake
find_package(tatami_eztimer CONFIG REQUIRED)
target_link_libraries(mylib INTERFACE tatami::eztimer)
```

### Manual

If you're not using CMake, the simple approach is to just copy the files the `include/` subdirectory -
either directly or with Git submodules - and include their path during compilation with, e.g., GCC's `-I`.
This also requires the external dependencies listed in [`extern/CMakeLists.txt`](extern/CMakeLists.txt) as well as Zlib.
