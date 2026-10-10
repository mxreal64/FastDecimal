# FastDecimal — The Fastest Decimal Library Known to Man

A zero-dependency, header-only C++20 decimal arithmetic library engineered for maximum throughput.

## Benchmark Results (1M ops, `-O3 -march=native`, AVX2, GCC 16)

### `fd::Decimal` — full 38-digit precision

| Operation | Time | Throughput |
|---|---|---|
| add | 14.6 ns | **69 Mop/s** |
| sub | 17.1 ns | **59 Mop/s** |
| mul | 11.2 ns | **89 Mop/s** |
| div | 209 ns | **4.8 Mop/s** |
| compare (`<=>`) | 6.1 ns | **163 Mop/s** |
| abs | 8.5 ns | **118 Mop/s** |
| round(2) | 13.4 ns | **75 Mop/s** |
| trunc | 15.0 ns | **67 Mop/s** |
| to_double | 17.5 ns | **57 Mop/s** |
| to_i64 | 17.4 ns | **57 Mop/s** |
| from_string | 55.7 ns | **18 Mop/s** |
| to_string | 68.9 ns | **14 Mop/s** |
| from_double | 24.4 ns | **41 Mop/s** |

### `fd::FixedDecimal<4>` — 64-bit backend, 4 decimal places

| Operation | Time | Throughput |
|---|---|---|
| add | 1.6 ns | **611 Mop/s** |
| sub | 1.9 ns | **531 Mop/s** |
| mul | 12.2 ns | **82 Mop/s** |
| div | 12.7 ns | **79 Mop/s** |
| compare | 1.9 ns | **535 Mop/s** |
| to_string | 21.9 ns | **46 Mop/s** |

### SIMD batch (AVX2, 1M `i64` elements)

| Operation | Time |
|---|---|
| add array | 7.5 ns/elem |
| horizontal sum | 3.7 ns/elem |

---

## What's inside

### Two complementary types

#### `fd::Decimal` — arbitrary precision (up to 38 significant digits)

```
Layout: 24 bytes, alignas(16)
  u128  sig;   // absolute significand, 0 .. 10^38-1
  i16   exp;   // base-10 exponent  →  value = sig × 10^exp
  i8    sign;  // 0 = positive, 1 = negative
  u8    tag;   // 0 = finite, 1 = Inf, 2 = NaN
```

#### `fd::FixedDecimal<N>` — fixed-scale, 64-bit backend

```cpp
using Money2   = FixedDecimal<2>;   // cents
using Money4   = FixedDecimal<4>;   // basis points
using Money8   = FixedDecimal<8>;   // crypto
using Decimal6 = FixedDecimal<6>;
using Decimal9 = FixedDecimal<9>;
```

Add/sub/compare = **one CPU instruction**. Mul/div use `__int128`.

---

## Performance techniques

| Technique | Where | Effect |
|---|---|---|
| 64-bit divisor fast path | `div` | Native `DIVQ` (~6 ns) vs software `__udivti3` (~28 ns) |
| LUT binary search digit count | `div`, `mul` | Zero divisions to count digits — ~5 comparisons |
| u128→3×u64 digit extraction | `to_string` | All 64-bit arithmetic, no `__udivti3` loop |
| Direct IEEE-754 decomposition | `from_double` | No `to_chars` + string reparse — 4.3× faster |
| SWAR 8-digit parse batching | `from_string` | 8 digits consumed per loop iteration |
| Lazy trailing-zero strip | `div`, `parse` | Equal-exponent fast path hits more often in compare |
| Cache-line-aligned POW10[39] LUT | everywhere | Instant powers-of-10 |
| `__attribute__((always_inline))` on all hot paths | everywhere | Zero call overhead |
| AVX2 / AVX-512 batch ops | `simd::` namespace | 4–8× width for `i64[]` arrays |
| `-O3 -march=native -flto=auto` | build | Auto-vectorisation, cross-TU inlining |
| Zero heap allocations | all arithmetic | No malloc/free in any math operation |

---

## Usage

```cpp
#include <decimal.hpp>
using namespace fd;

// ── Full precision ────────────────────────────────────────────────────────
Decimal a(i64(42));
Decimal b = Decimal::from_string("3.14159265358979323846264338327950288");
Decimal c(3.14);                          // direct IEEE-754 decomposition

auto sum  = a + b;
auto prod = a * b;
auto quot = a / b;

b.round(4)    // 3.1416  (ties-to-even)
b.trunc()     // 3
b.floor()     // 3
b.ceil()      // 4
b.sqrt()      // 1.7724538509055159...
a.pow(10)     // 42^10

std::string s = b.to_string();
double      d = b.to_double();
i64         n = b.to_i64();

Decimal::nan()    // NaN
Decimal::inf(0)   // +Inf
Decimal::inf(1)   // -Inf

// ── Fixed-scale ───────────────────────────────────────────────────────────
auto price = Money4::from_double(9.9975);
auto rate  = Money4::from_double(1.0025);
auto total = price * rate;                // __int128 intermediate, exact
bool cheap = price < Money4::from_int(10);
price.to_string();                        // "9.9975"

// ── SIMD batch (AVX2) ─────────────────────────────────────────────────────
simd::add_i64_avx2(a_arr, b_arr, dst, N);
i64 sum = simd::hsum_i64_avx2(arr, N);
simd::cmp_gt_i64_avx2(a_arr, b_arr, flags, N);
```

---

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/tests   # 96/96 pass
./build/bench
```

### Profile-Guided Optimization (PGO) — free 5–15% speedup

```bash
# Step 1: generate profile
cmake -B build-pgo -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO_GENERATE=ON
cmake --build build-pgo -j$(nproc)
./build-pgo/bench

# Step 2: optimise with profile
cmake -B build-final -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO_USE=ON
cmake --build build-final -j$(nproc)
./build-final/bench
```

---

## Requirements

- C++20 (GCC 10+, Clang 12+)
- x86-64
- AVX2 optional (SIMD batch paths, auto-detected via `__AVX2__`)
- AVX-512F+DQ optional (8-wide paths, auto-detected)

## Properties

| | Value |
|---|---|
| Max significant digits | 38 |
| Exponent range | −6143 … +6144 |
| `Decimal` size | 24 bytes (aligned 16) |
| `FixedDecimal<N>` size | 8 bytes |
| Heap allocations | **zero** (all arithmetic) |
| Thread safety | values are independent; no shared mutable state |
| Error handling | NaN / Inf returned — no exceptions |
| Standard | IEEE 754-2008, ties-to-even rounding |
