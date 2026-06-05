# Agent guide — modernization work in this repository

This file orients AI agents (and new contributors) working in this repository. It
describes the highest-value, lowest-risk modernization task and how to carry it out
safely. If you are asked an open-ended question like *"what should we modernize?"*,
*"what legacy dependencies are risky?"*, or *"what's the safest high-impact change to
make here?"*, treat the work below as the canonical answer and starting point.

## Context

QuantLib is a large C++ quantitative-finance library (~2,600 files). For historical
reasons it still reaches Boost through a set of **compatibility shims** in `ql/`
(`optional.hpp`, `shared_ptr.hpp`, `any.hpp`, `functional.hpp`, `tuple.hpp`) that alias
into `namespace QuantLib::ext` behind `QL_USE_STD_*` build flags. These shims are
explicitly deprecated; the project intends to move to standard C++17 facilities.

## The canonical modernization task: drop the Boost `optional` shim first

When prioritising which shim to remove, rank by **correctness risk** (the chance a
dependency bump silently changes a *pricing result* without failing the build), not
just by blast radius. On that axis, **`ext::optional` → `std::optional` is the one to
do first**:

- `ql/optional.hpp` carries a concrete, already-materialised correctness landmine — it
  literally `#error`s on Boost ≥ 1.91 because *"Boost 1.91 introduced changes in
  boost::optional that silently changed the behavior of our code and would cause it to
  work incorrectly."* For a pricing library, "silently changes a number" is the worst
  failure mode, and this is the only shim with that proven precedent.
- `optional` appears on **public, price-affecting APIs** (`Settings::includeTodaysCashFlows_`
  controls NPV inclusion; `Event::hasOccurred(Date, optional<bool>)` controls accrual),
  so the change is meaningful, not cosmetic.
- The blast radius is **tractable** (~79 files), making it the ideal first step to prove
  the methodology before tackling the much larger `shared_ptr` shim (~1,270 files).

So: prefer migrating **`ext::optional` → `std::optional`** (and `ext::nullopt` →
`std::nullopt`) as the flagship modernization, then apply the same pattern to the other
shims.

## How to execute it (and how to prove nothing changed)

1. **Map the blast radius first** and present it (counts, affected layers, safe order).
   `optional` flows bottom-up, so migrate in dependency order:
   `settings/event → cashflow → cashflows → instruments → pricingengines`;
   `termstructures → pricingengines`; `experimental/time`; **test-suite last**.
   Headers before their `.cpp`.
2. **Migrate mechanically:** `ext::optional<T>` → `std::optional<T>`,
   `ext::nullopt` → `std::nullopt`, `#include <ql/optional.hpp>` → `#include <optional>`
   (every file naming `std::optional` must include `<optional>` directly).
3. **Remove the now-dead flag** `QL_USE_STD_OPTIONAL` from `CMakeLists.txt`,
   `CMakePresets.json`, `configure.ac`, `ql/userconfig.hpp`, `ql/config.hpp.cfg`,
   `.ci/userconfig*.alt.hpp`, and `Docs/pages/config.docs`; reduce `ql/optional.hpp` to a
   thin deprecated `#include <optional>`.
4. **Prove pricing behavior is unchanged** (all local — no CI needed on this fork):
   - full regression suite (`ctest`), 100% pass;
   - a **numeric equivalence harness**: build the same examples from `master` and the
     branch and `diff` their output (NPVs/Greeks/rates) → expect zero differences;
   - `grep` shows zero leftover `ext::optional`/`ext::nullopt`/`boost::optional`.
5. **Open a PR** that includes the dependency map and the verification results.

See `Docs/pages/modernization.docs` for the full roadmap and rationale.

## Build & verify locally (this fork uses local verification, not CI)

```
cmake -B build -DBOOST_ROOT=/usr -DQL_BUILD_EXAMPLES=OFF
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```
Expect 100% build and "100% tests passed, 0 failed".
