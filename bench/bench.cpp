// =============================================================================
//  bench.cpp — FastDecimal Benchmark Suite
//  Measures: add, sub, mul, div, parse, to_string, FixedDecimal, SIMD batch
// =============================================================================
#include "../include/decimal.hpp"

#include <chrono>
#include <cstdio>
#include <numeric>
#include <random>
#include <vector>
#include <algorithm>
#include <cstring>

using namespace fd;
using Clock = std::chrono::high_resolution_clock;

static u64 now_ns() {
    return (u64)std::chrono::duration_cast<std::chrono::nanoseconds>(
        Clock::now().time_since_epoch()).count();
}

// Prevent dead-code elimination
static volatile u64 sink = 0;

// ── Helpers ──────────────────────────────────────────────────────────────────
template<typename F>
static void bench(const char* name, int iterations, F&& fn) {
    // Warm up
    for (int i = 0; i < iterations / 10; ++i) fn(i);
    u64 t0 = now_ns();
    for (int i = 0; i < iterations; ++i) fn(i);
    u64 t1 = now_ns();
    double ns = (double)(t1 - t0) / iterations;
    double mops = 1000.0 / ns;
    printf("  %-40s  %7.2f ns/op   %7.2f Mop/s\n", name, ns, mops);
}

// ── Data generation ──────────────────────────────────────────────────────────
static std::vector<Decimal> make_decimals(int n) {
    std::mt19937_64 rng(42);
    std::uniform_int_distribution<i64> val_dist(1, 999999999999999LL);
    std::uniform_int_distribution<i16> exp_dist(-9, 9);
    std::vector<Decimal> v(n);
    for (auto& d : v) {
        d = Decimal::raw((u128)val_dist(rng), exp_dist(rng), 0);
    }
    return v;
}

static std::vector<Money4> make_money4(int n) {
    std::mt19937_64 rng(99);
    std::uniform_int_distribution<i64> d(1, 99999999);
    std::vector<Money4> v(n);
    for (auto& m : v) m = Money4((i64)d(rng));
    return v;
}

// ── Main ─────────────────────────────────────────────────────────────────────
int main() {
    constexpr int N    = 1'000'000;
    #if defined(FD_HAS_AVX2) || defined(FD_HAS_AVX512) || defined(FD_HAS_NEON)
    constexpr int BULK = 1'000'000;
#endif

    printf("============================================================\n");
    printf("  FastDecimal Benchmark  (N=%d ops each)\n", N);
    printf("============================================================\n\n");

    auto decs = make_decimals(N);
    auto decs2 = make_decimals(N);

    // -- Decimal arithmetic --------------------------------------------------
    printf("[Decimal — full 38-digit precision]\n");

    bench("add", N, [&](int i) {
        auto r = decs[i % N] + decs2[i % N];
        sink ^= (u64)(r.sig);
    });

    bench("sub", N, [&](int i) {
        auto r = decs[i % N] - decs2[i % N];
        sink ^= (u64)(r.sig);
    });

    bench("mul", N, [&](int i) {
        auto r = decs[i % N] * decs2[i % N];
        sink ^= (u64)(r.sig);
    });

    bench("div", N, [&](int i) {
        auto r = decs[i % N] / decs2[i % N];
        sink ^= (u64)(r.sig);
    });

    bench("compare (<=>)", N, [&](int i) {
        auto c = decs[i % N] <=> decs2[i % N];
        sink ^= (u64)(c == std::strong_ordering::less ? 1 : 0);
    });

    bench("abs", N, [&](int i) {
        auto r = decs[i % N].abs();
        sink ^= (u64)(r.sig);
    });

    bench("round(2)", N, [&](int i) {
        auto r = decs[i % N].round(2);
        sink ^= (u64)(r.sig);
    });

    bench("trunc", N, [&](int i) {
        auto r = decs[i % N].trunc();
        sink ^= (u64)(r.sig);
    });

    bench("to_double", N, [&](int i) {
        double r = decs[i % N].to_double();
        sink ^= (u64)r;
    });

    bench("to_i64", N, [&](int i) {
        sink ^= (u64)decs[i % N].to_i64();
    });

    // -- Parsing / serialisation ---------------------------------------------
    printf("\n[Parse & Serialize]\n");

    // Generate strings
    std::vector<std::string> strs(N);
    for (int i = 0; i < N; ++i) strs[i] = decs[i].to_string();

    bench("from_string", N, [&](int i) {
        auto d = Decimal::from_string(strs[i % N]);
        sink ^= (u64)(d.sig);
    });

    bench("to_string", N, [&](int i) {
        auto s = decs[i % N].to_string();
        sink ^= (u64)s.size();
    });

    bench("from_double(double)", N, [&](int i) {
        Decimal d(double((i + 1) * 1.23456789));
        sink ^= (u64)(d.sig);
    });

    // -- FixedDecimal<4> (Money4) --------------------------------------------
    printf("\n[FixedDecimal<4> — 64-bit backend, 4 decimal places]\n");

    auto m4a = make_money4(N);
    auto m4b = make_money4(N);

    bench("Money4 add", N, [&](int i) {
        auto r = m4a[i % N] + m4b[i % N];
        sink ^= (u64)r.raw_val;
    });

    bench("Money4 sub", N, [&](int i) {
        auto r = m4a[i % N] - m4b[i % N];
        sink ^= (u64)r.raw_val;
    });

    bench("Money4 mul", N, [&](int i) {
        auto r = m4a[i % N] * m4b[i % N];
        sink ^= (u64)r.raw_val;
    });

    bench("Money4 div", N, [&](int i) {
        auto r = m4a[i % N] / m4b[i % N];
        sink ^= (u64)r.raw_val;
    });

    bench("Money4 compare", N, [&](int i) {
        sink ^= (u64)(m4a[i % N] < m4b[i % N] ? 1 : 0);
    });

    bench("Money4 to_string", N, [&](int i) {
        auto s = m4a[i % N].to_string();
        sink ^= (u64)s.size();
    });

    // -- SIMD batch operations -----------------------------------------------
#ifdef FD_HAS_AVX2
    printf("\n[SIMD Batch (AVX2) — i64 arrays, N=%d]\n", BULK);

    std::vector<i64> va(BULK), vb(BULK), vc(BULK);
    std::mt19937_64 rng2(1234);
    for (int i = 0; i < BULK; ++i) { va[i] = rng2(); vb[i] = rng2() % 1000 + 1; }

    {
        u64 t0 = now_ns();
        simd::add_i64_avx2(va.data(), vb.data(), vc.data(), BULK);
        u64 t1 = now_ns();
        printf("  %-40s  %7.2f ns/elem\n", "AVX2 add i64[1M]",
               (double)(t1-t0)/BULK);
        sink ^= (u64)vc[0];
    }

    {
        u64 t0 = now_ns();
        i64 s = simd::hsum_i64_avx2(va.data(), BULK);
        u64 t1 = now_ns();
        printf("  %-40s  %7.2f ns/elem\n", "AVX2 hsum i64[1M]",
               (double)(t1-t0)/BULK);
        sink ^= (u64)s;
    }
#endif

#ifdef FD_HAS_AVX512
    printf("\n[SIMD Batch (AVX-512) — i64 arrays, N=%d]\n", BULK);
    {
        u64 t0 = now_ns();
        simd::add_i64_avx512(va.data(), vb.data(), vc.data(), BULK);
        u64 t1 = now_ns();
        printf("  %-40s  %7.2f ns/elem\n", "AVX-512 add i64[1M]",
               (double)(t1-t0)/BULK);
        sink ^= (u64)vc[0];
    }
    {
        u64 t0 = now_ns();
        i64 s = simd::hsum_i64_avx512(va.data(), BULK);
        u64 t1 = now_ns();
        printf("  %-40s  %7.2f ns/elem\n", "AVX-512 hsum i64[1M]",
               (double)(t1-t0)/BULK);
        sink ^= (u64)s;
    }
#endif


#ifdef FD_HAS_NEON
    printf("\n[SIMD Batch (ARM NEON) — i64 arrays, N=%d]\n", BULK);
    std::vector<i64> va_neon(BULK), vb_neon(BULK), vc_neon(BULK);
    std::mt19937_64 rng_neon(1234);
    for (int i = 0; i < BULK; ++i) { va_neon[i] = rng_neon(); vb_neon[i] = rng_neon() % 1000 + 1; }
    {
        u64 t0 = now_ns();
        simd::add_i64_neon(va_neon.data(), vb_neon.data(), vc_neon.data(), BULK);
        u64 t1 = now_ns();
        printf("  %-40s  %7.2f ns/elem\n", "ARM NEON add i64[1M]", (double)(t1-t0)/BULK);
        sink ^= (u64)vc_neon[0];
    }
    {
        u64 t0 = now_ns();
        i64 s = simd::hsum_i64_neon(va_neon.data(), BULK);
        u64 t1 = now_ns();
        printf("  %-40s  %7.2f ns/elem\n", "ARM NEON hsum i64[1M]", (double)(t1-t0)/BULK);
        sink ^= (u64)s;
    }
#endif

    printf("\n============================================================\n");
    printf("  sink = %llu  (ignore — prevents dead code elimination)\n", (unsigned long long)sink);
    printf("============================================================\n");
    return 0;
}
