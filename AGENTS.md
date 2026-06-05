# Agent guide — modernization work in this repository

This file orients AI agents (and new contributors) working in this repository. It
describes the highest-value, lowest-risk modernization tasks and how to carry them out
safely.

## Context

QuantLib is a large C++ quantitative-finance library (~2,600 files). For historical
reasons it still reaches Boost through a set of **compatibility shims** in `ql/`
(`shared_ptr.hpp`, `any.hpp`, `functional.hpp`, `tuple.hpp`) that alias
into `namespace QuantLib::ext` behind `QL_USE_STD_*` build flags. These shims are
explicitly deprecated; the project intends to move to standard C++17 facilities.

## Completed: `ext::optional` → `std::optional`

The `optional` shim (`ql/optional.hpp`) has been fully migrated. All 80 source files
now use `std::optional` / `std::nullopt` directly. The `QL_USE_STD_OPTIONAL` build
flag has been removed. `ql/optional.hpp` is now a thin deprecated redirect to
`<optional>` for backward compatibility.

This was prioritized first because `ql/optional.hpp` carried a proven
silent-correctness landmine (`#error` on Boost ≥ 1.91 due to silently changed
`boost::optional` behavior). The migration was verified with the full regression
suite (100% pass) and numeric equivalence harness (zero pricing differences).

## Next modernization tasks

Apply the same methodology to the remaining shims, in this order:

1. **`ext::any` → `std::any`** — smallest blast radius among the remaining shims.
2. **`ext::function`/`ext::bind`/`ext::tuple` → `std::` equivalents** — moderate blast radius.
3. **`ext::shared_ptr` → `std::shared_ptr`** — largest blast radius (~1,270 files); tackle last.

### How to execute a shim removal

1. **Map the blast radius first** and present it (counts, affected layers, safe order).
   Migrate in dependency order (lower layers first). Headers before their `.cpp`.
2. **Migrate mechanically:** `ext::<name>` → `std::<name>`,
   `#include <ql/<shim>.hpp>` → `#include <std header>`
   (every file naming `std::<name>` must include the std header directly).
3. **Remove the now-dead flag** `QL_USE_STD_*` from `CMakeLists.txt`,
   `CMakePresets.json`, `configure.ac`, `ql/userconfig.hpp`, `ql/config.hpp.cfg`,
   `.ci/userconfig*.alt.hpp`, and `Docs/pages/config.docs`; reduce the shim header to a
   thin deprecated redirect.
4. **Prove pricing behavior is unchanged** (all local — no CI needed on this fork):
   - full regression suite (`ctest`), 100% pass;
   - a **numeric equivalence harness**: build the same examples from `master` and the
     branch and `diff` their output (NPVs/Greeks/rates) → expect zero differences;
   - `grep` shows zero leftover shim-aliased usages.
5. **Open a PR** that includes the dependency map and the verification results.

See `Docs/pages/modernization.docs` for the full roadmap and rationale.

## Build & verify locally (this fork uses local verification, not CI)

```
cmake -B build -DBOOST_ROOT=/usr -DQL_BUILD_EXAMPLES=OFF
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```
Expect 100% build and "100% tests passed, 0 failed".
