// =============================================================================
//  tests.cpp — FastDecimal Unit Test Suite
// =============================================================================
#include "../include/decimal.hpp"
#include <cstdio>
#include <cmath>
#include <string>
#include <sstream>
#include <unordered_map>

using namespace fd;

static int pass = 0, fail = 0;

#define CHECK(expr) do { \
    if (expr) { ++pass; } \
    else { ++fail; printf("FAIL [%s:%d]: %s\n", __FILE__, __LINE__, #expr); } \
} while(0)

#define CHECK_STR(a, b) do { \
    auto sa = (a); auto sb = std::string(b); \
    if (sa == sb) { ++pass; } \
    else { ++fail; printf("FAIL [%s:%d]: \"%s\" != \"%s\"\n", __FILE__, __LINE__, sa.c_str(), sb.c_str()); } \
} while(0)

static Decimal D(i64 v) { return Decimal(v); }

// ── Basic construction ───────────────────────────────────────────────────────
void test_construction() {
    Decimal z;
    CHECK(z.is_zero());

    Decimal i64v(i64(42));
    CHECK(i64v.to_i64() == 42);

    Decimal neg(i64(-7));
    CHECK(neg.to_i64() == -7);
    CHECK(neg.is_neg());

    Decimal u64v((u64)999999999999999999ULL);
    CHECK(u64v.to_i64() == (i64)999999999999999999ULL);

    CHECK(Decimal::nan().is_nan());
    CHECK(Decimal::inf(0).is_inf());
    CHECK(!Decimal::inf(0).is_neg());
    CHECK(Decimal::inf(1).is_neg());
}

// ── Parsing ──────────────────────────────────────────────────────────────────
void test_parse() {
    auto chk = [](const char* s, const char* expected) {
        auto d = Decimal::from_string(s);
        auto out = d.to_string();
        if (out != expected) {
            ++fail;
            printf("FAIL parse: \"%s\" -> \"%s\", expected \"%s\"\n", s, out.c_str(), expected);
        } else { ++pass; }
    };

    chk("0",          "0");
    chk("123",        "123");
    chk("-456",       "-456");
    chk("3.14",       "3.14");
    chk("-0.001",     "-0.001");
    chk("1e5",        "100000");
    chk("1.5e2",      "150");
    chk("1.23e-4",    "0.000123");
    chk("9999999999999999999", "9999999999999999999");
    chk("NaN",        "NaN");
    chk("Inf",        "Inf");
    chk("-Inf",       "-Inf");
}

// ── Arithmetic ───────────────────────────────────────────────────────────────
void test_add() {
    CHECK((D(3) + D(4)).to_i64() == 7);
    CHECK((D(10) + D(-3)).to_i64() == 7);
    CHECK((D(-5) + D(-2)).to_i64() == -7);
    CHECK((D(0) + D(99)).to_i64() == 99);

    // Fractional
    auto a = Decimal::from_string("1.5");
    auto b = Decimal::from_string("2.5");
    CHECK_STR((a + b).to_string(), "4");

    auto c = Decimal::from_string("0.1");
    auto d = Decimal::from_string("0.2");
    auto e = c + d;
    CHECK_STR(e.round(1).to_string(), "0.3");
}

void test_sub() {
    CHECK((D(10) - D(3)).to_i64() == 7);
    CHECK((D(3) - D(10)).to_i64() == -7);
    CHECK((D(-5) - D(-2)).to_i64() == -3);
    auto a = Decimal::from_string("10.5");
    auto b = Decimal::from_string("0.5");
    CHECK_STR((a - b).to_string(), "10");
}

void test_mul() {
    CHECK((D(6) * D(7)).to_i64() == 42);
    CHECK((D(-3) * D(4)).to_i64() == -12);
    CHECK((D(-3) * D(-4)).to_i64() == 12);

    auto a = Decimal::from_string("1.5");
    auto b = Decimal::from_string("4");
    CHECK_STR((a * b).to_string(), "6");

    // Large
    auto big = Decimal::from_string("999999999999999999");
    auto two = D(2);
    CHECK_STR((big * two).to_string(), "1999999999999999998");
}

void test_div() {
    CHECK((D(42) / D(6)).to_i64() == 7);
    CHECK((D(10) / D(4)).to_string() == Decimal::from_string("2.5").to_string());

    // 1/3 should give ~38 significant digits
    auto one_third = D(1) / D(3);
    auto s = one_third.to_string();
    CHECK(s.size() > 35);  // should have lots of 3s

    // Division by zero → Inf
    auto inf_r = D(5) / D(0);
    CHECK(inf_r.is_inf());

    // 0/0 → NaN
    auto nan_r = D(0) / D(0);
    CHECK(nan_r.is_nan());
}

// ── Comparison ───────────────────────────────────────────────────────────────
void test_cmp() {
    CHECK(D(1) < D(2));
    CHECK(D(2) > D(1));
    CHECK(D(1) == D(1));
    CHECK(D(-1) < D(0));
    CHECK(D(0) == D(0));

    // Different scales, same value
    auto a = Decimal::from_string("1.50");
    auto b = Decimal::from_string("1.5");
    CHECK(a == b);

    auto c = Decimal::from_string("0.1");
    auto d = Decimal::from_string("0.10");
    CHECK(c == d);
}

// ── Rounding ─────────────────────────────────────────────────────────────────
void test_round() {
    auto a = Decimal::from_string("3.14159");
    CHECK_STR(a.round(2).to_string(), "3.14");
    CHECK_STR(a.round(4).to_string(), "3.1416");
    CHECK_STR(a.round(0).to_string(), "3");

    auto b = Decimal::from_string("2.5");
    CHECK_STR(b.round(0).to_string(), "2");  // ties-to-even: 2 is even

    auto c = Decimal::from_string("3.5");
    CHECK_STR(c.round(0).to_string(), "4");  // ties-to-even: 4 is even

    CHECK_STR(Decimal::from_string("1.23").trunc().to_string(), "1");
    CHECK_STR(Decimal::from_string("-1.7").floor().to_string(), "-2");
    CHECK_STR(Decimal::from_string("1.2").ceil().to_string(),   "2");
}

// ── Power ────────────────────────────────────────────────────────────────────
void test_pow() {
    CHECK((D(2).pow(10)).to_i64() == 1024);
    CHECK((D(3).pow(0)).to_i64()  == 1);
    CHECK((D(2).pow(1)).to_i64()  == 2);
    CHECK((D(10).pow(6)).to_i64() == 1000000);

    // Negative power: 2^-1 = 0.5
    auto half = D(2).pow(-1);
    CHECK_STR(half.to_string(), "0.5");
}

// ── FixedDecimal ─────────────────────────────────────────────────────────────
void test_fixed() {
    auto a = Money4::from_int(10);
    auto b = Money4::from_int(3);

    CHECK((a + b).to_int() == 13);
    CHECK((a - b).to_int() == 7);
    CHECK((a * b).to_int() == 30);
    CHECK((a / b).to_string() == "3.3333");

    auto c = Money4::from_double(1.5);
    auto d = Money4::from_double(2.5);
    CHECK((c + d).to_int() == 4);
    CHECK(c < d);
    CHECK(d > c);
    CHECK(c == Money4::from_double(1.5));

    // String
    CHECK(Money4::from_int(42).to_string() == "42.0000");
    CHECK(Money4::from_double(3.14).to_string() == "3.1400");

    // Money2 (cents): 9.99 * 0.10 = 0.9990 → truncates to 0.99 in fixed-scale
    auto price = Money2::from_double(9.99);
    auto tax   = Money2::from_double(0.10);
    auto result = price * tax;
    // 999 * 10 / 100 = 99 raw → 0.99
    CHECK_STR(result.to_string(), "0.99");
}

// ── Special values ───────────────────────────────────────────────────────────
void test_special() {
    auto nan = Decimal::nan();
    auto inf  = Decimal::inf(0);
    auto ninf = Decimal::inf(1);

    CHECK((nan + D(1)).is_nan());
    CHECK((D(1) + nan).is_nan());
    CHECK((nan * D(1)).is_nan());
    CHECK((inf + inf).is_inf());
    CHECK((inf + ninf).is_nan());
    CHECK((inf * D(-1)).is_inf());
    CHECK((inf * D(-1)).is_neg());
}

// ── SIMD batch ───────────────────────────────────────────────────────────────
void test_simd() {
#ifdef FD_HAS_AVX2
    constexpr int N = 16;
    i64 a[N], b[N], c[N];
    for (int i = 0; i < N; ++i) { a[i] = i * 100; b[i] = (N - i) * 50; }
    simd::add_i64_avx2(a, b, c, N);
    for (int i = 0; i < N; ++i) {
        CHECK(c[i] == a[i] + b[i]);
    }

    i64 sum = simd::hsum_i64_avx2(a, N);
    i64 expected = 0;
    for (int i = 0; i < N; ++i) expected += a[i];
    CHECK(sum == expected);
    printf("  [AVX2 SIMD batch tests passed]\n");
#else
    printf("  [SIMD tests skipped — no AVX2]\n");
#endif
}

// ── Main ─────────────────────────────────────────────────────────────────────
int main() {
    printf("============================================================\n");
    printf("  FastDecimal Unit Tests\n");
    printf("============================================================\n\n");

    test_construction();
    test_parse();
    test_add();
    test_sub();
    test_mul();
    test_div();
    test_cmp();
    test_round();
    test_pow();
    test_fixed();
    test_special();
    test_simd();

    // ── Streams & Formatting ─────────────────────────────────────────────────
    {
        std::stringstream ss;
        ss << Decimal::from_string("123.456");
        CHECK_STR(ss.str(), "123.456");

        std::stringstream ss_fix;
        ss_fix << Money4::from_double(12.34);
        CHECK_STR(ss_fix.str(), "12.3400");
    }

    // ── std::hash & Containers ───────────────────────────────────────────────
    {
        std::unordered_map<Decimal, int> map;
        map[Decimal::from_string("10.5")] = 100;
        map[Decimal::from_string("20.25")] = 200;
        CHECK(map[Decimal::from_string("10.5")] == 100);
        CHECK(map[Decimal::from_string("20.25")] == 200);

        std::unordered_map<Money4, std::string> fix_map;
        fix_map[Money4::from_double(5.5)] = "five point five";
        CHECK_STR(fix_map[Money4::from_double(5.5)], "five point five");
    }

    // ── Edge Cases: Malformed Strings & Parser Robustness ───────────────────
    {
        CHECK(Decimal::from_string("").is_nan());
        CHECK(Decimal::from_string("+").is_nan());
        CHECK(Decimal::from_string("-").is_nan());
        CHECK(Decimal::from_string(".").is_nan());
        CHECK(Decimal::from_string("..").is_nan());
        CHECK(Decimal::from_string("abc").is_nan());
        CHECK(Decimal::from_string("1e").is_nan());
        CHECK(Decimal::from_string("1e+").is_nan());

        // Extreme exponents
        CHECK(Decimal::from_string("1e7000").is_inf());
        CHECK(Decimal::from_string("1e-7000").is_zero());

        // 38-digit full significand
        auto max_dig = Decimal::from_string("99999999999999999999999999999999999999");
        CHECK(max_dig.is_finite());
        CHECK(!max_dig.is_zero());
    }

    // ── Arithmetic Scale Alignment Safety ───────────────────────────────────
    {
        auto huge = Decimal::from_string("99999999999999999999999999999999999999");
        auto tiny = Decimal::from_string("0.00000000000000000000000000000000000001");
        auto sum = huge + tiny;
        CHECK(sum.is_finite());
        CHECK(huge > tiny);
        CHECK(tiny < huge);
    }

    // ── SIMD unified dispatch ────────────────────────────────────────────────
    {
        constexpr int N = 32;
        i64 a[N], b[N], c[N];
        i8 cmp[N];
        for (int i = 0; i < N; ++i) { a[i] = i * 2; b[i] = i; }
        simd::add_i64_batch(a, b, c, N);
        for (int i = 0; i < N; ++i) CHECK(c[i] == a[i] + b[i]);

        i64 total = simd::hsum_i64_batch(a, N);
        i64 expected_total = 0;
        for (int i = 0; i < N; ++i) expected_total += a[i];
        CHECK(total == expected_total);

        simd::cmp_gt_i64_batch(a, b, cmp, N);
        for (int i = 0; i < N; ++i) CHECK(cmp[i] == (a[i] > b[i] ? 1 : 0));
    }

    printf("\n============================================================\n");
    printf("  Results: %d passed, %d failed\n", pass, fail);
    printf("============================================================\n");
    return fail == 0 ? 0 : 1;
}
