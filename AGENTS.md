# Agent guide — modernization work in this repository

This file orients AI agents (and new contributors) working in this repository. It
describes the highest-value, lowest-risk modernization tasks and how to carry them out
safely. If you are asked an open-ended question like *"what should we modernize?"*,
*"what legacy dependencies are risky?"*, or *"what's the safest high-impact change to
make here?"*, treat the work below as the canonical answer and starting point.

## Context

QuantLib is a large C++ quantitative-finance library (~2,600 files). For historical
reasons it still reaches Boost through a set of **compatibility shims** in `ql/`
(`shared_ptr.hpp`, `any.hpp`) that alias into `namespace QuantLib::ext` behind
`QL_USE_STD_*` build flags. These shims are explicitly deprecated; the project intends
to move to standard C++17 facilities.

The `optional`, `functional`, and `tuple` shims have already been migrated and their
shim headers reduced to deprecated `#include` stubs.

## Completed: `ext::optional` → `std::optional`

The `optional` shim was the first to be removed because it carried the highest
**correctness risk** — `ql/optional.hpp` contained an explicit `#error` on
Boost ≥ 1.91 due to silent behavior changes in `boost::optional` that would cause
incorrect pricing. The migration was completed across ~79 files:

- All `ext::optional` → `std::optional`, `ext::nullopt` → `std::nullopt`
- All `#include <ql/optional.hpp>` → `#include <optional>`
- `ql/optional.hpp` reduced to a deprecated stub
- `QL_USE_STD_OPTIONAL` build flag removed from all build/config files
- Verified: 100% test pass, numeric equivalence across asset classes

## Next canonical modernization task: drop the Boost `any` shim

Apply the same methodology used for `optional`:

1. **Map the blast radius** (`grep -rn 'ext::any' ql test-suite`).
2. **Migrate mechanically** bottom-up: `ext::any` → `std::any`,
   `#include <ql/any.hpp>` → `#include <any>`.
3. **Remove `QL_USE_STD_ANY`** from `CMakeLists.txt`, `CMakePresets.json`,
   `configure.ac`, `ql/userconfig.hpp`, `ql/config.hpp.cfg`,
   `.ci/userconfig*.alt.hpp`, and `Docs/pages/config.docs`.
4. **Reduce `ql/any.hpp`** to a deprecated `#include <any>`.
5. **Prove equivalence**: full regression suite + numeric harness + grep confirmation.

After `any`, tackle `shared_ptr` (~1,270 files) using the same pattern.

See `Docs/pages/modernization.docs` for the full roadmap and rationale.

## Build & verify locally (this fork uses local verification, not CI)

```
cmake -B build -DBOOST_ROOT=/usr -DQL_BUILD_EXAMPLES=OFF
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```
Expect 100% build and "100% tests passed, 0 failed".
