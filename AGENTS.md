# Agent guide — modernization work in this repository

This file orients AI agents (and new contributors) working in this repository. It
describes the modernization status and the next high-value task.

## Context

QuantLib is a large C++ quantitative-finance library (~2,600 files). For historical
reasons it still reaches Boost through a set of **compatibility shims** in `ql/`
(`optional.hpp`, `shared_ptr.hpp`, `any.hpp`, `functional.hpp`, `tuple.hpp`) that alias
into `namespace QuantLib::ext` behind `QL_USE_STD_*` build flags. These shims are
explicitly deprecated; the project intends to move to standard C++17 facilities.

## Completed: `ext::optional` → `std::optional`

The `optional` shim migration is **complete**. All `ext::optional` / `ext::nullopt`
usages across ~79 files have been replaced with `std::optional` / `std::nullopt`.
The `QL_USE_STD_OPTIONAL` build flag has been removed from all config files, and
`ql/optional.hpp` is now a tombstoned deprecated header (like `functional.hpp` and
`tuple.hpp`).

This was the highest-priority migration because:
- `ql/optional.hpp` carried a `#error` on Boost ≥ 1.91 due to silent
  pricing-correctness breakage.
- `optional` appeared on **public, price-affecting APIs**
  (`Settings::includeTodaysCashFlows_`, `Event::hasOccurred`).

## Next modernization task: drop the Boost `any` shim, then `shared_ptr`

The next shim to remove is `ext::any` → `std::any`, followed by the much larger
`shared_ptr` shim (~1,270 files). Apply the same methodology proven with `optional`:

1. **Map the blast radius** and present it (counts, affected layers, safe order).
2. **Migrate mechanically** in dependency order (bottom-up).
3. **Remove the now-dead `QL_USE_STD_*` flag** from `CMakeLists.txt`,
   `CMakePresets.json`, `configure.ac`, `ql/userconfig.hpp`, `ql/config.hpp.cfg`,
   `.ci/userconfig*.alt.hpp`, and `Docs/pages/config.docs`; reduce the shim header
   to a thin deprecated `#include`.
4. **Prove pricing behavior is unchanged** (all local — no CI needed on this fork):
   - full regression suite (`ctest`), 100% pass;
   - a **numeric equivalence harness**: build the same examples from `master` and the
     branch and `diff` their output (NPVs/Greeks/rates) → expect zero differences;
   - `grep` shows zero leftover shim usages.
5. **Open a PR** that includes the dependency map and the verification results.

See `Docs/pages/modernization.docs` for the full roadmap and rationale.

## Build & verify locally (this fork uses local verification, not CI)

```
cmake -B build -DBOOST_ROOT=/usr -DQL_BUILD_EXAMPLES=OFF
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```
Expect 100% build and "100% tests passed, 0 failed".
