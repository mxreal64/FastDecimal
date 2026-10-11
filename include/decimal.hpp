// =============================================================================
//  decimal.hpp — FastDecimal: Cross-Platform x86-64 (AVX2/AVX-512) & ARM64 (NEON)
// =============================================================================
#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cassert>
#include <compare>
#include <string>
#include <string_view>
#include <charconv>
#include <array>
#include <span>
#include <bit>
#include <cmath>

// Architecture detection & SIMD intrinsics
#if defined(__x86_64__) || defined(_M_X64)
#  include <immintrin.h>
#  if defined(__AVX512F__) && defined(__AVX512DQ__)
#    define FD_HAS_AVX512 1
#  endif
#  if defined(__AVX2__)
#    define FD_HAS_AVX2 1
#  endif
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__ARM_NEON)
#  include <arm_neon.h>
#  define FD_HAS_NEON 1
#endif

#if defined(__GNUC__) || defined(__clang__)
#  define FD_ALWAYS_INLINE  __attribute__((always_inline)) inline
#  define FD_NOINLINE       __attribute__((noinline))
#  define FD_LIKELY(x)      __builtin_expect(!!(x), 1)
#  define FD_UNLIKELY(x)    __builtin_expect(!!(x), 0)
#  define FD_RESTRICT       __restrict__
#else
#  define FD_ALWAYS_INLINE  __forceinline
#  define FD_NOINLINE       __declspec(noinline)
#  define FD_LIKELY(x)      (x)
#  define FD_UNLIKELY(x)    (x)
#  define FD_RESTRICT
#endif

namespace fd {

using i128 = __int128;
using u128 = unsigned __int128;
using i64  = std::int64_t;
using u64  = std::uint64_t;
using i32  = std::int32_t;
using u32  = std::uint32_t;
using i16  = std::int16_t;
using u16  = std::uint16_t;
using i8   = std::int8_t;
using u8   = std::uint8_t;

static constexpr int  MAX_DIGITS = 38;
static constexpr int  SCALE_MIN  = -6143;
static constexpr int  SCALE_MAX  =  6144;
static constexpr u128 SIG_MAX = ((u128)5421010862427522170ULL << 64) | 687399551400673279ULL;

// Two-digit LUT for serialization
alignas(64) inline constexpr char DIGIT_PAIRS[201] =
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";

// Power-of-10 LUTs
struct alignas(64) Pow10Table {
    u128 v[40];
    constexpr Pow10Table():v{}{v[0]=1;for(int i=1;i<=39;++i)v[i]=v[i-1]*10;}
};
inline constexpr Pow10Table POW10{};

alignas(64) inline constexpr std::array<u64,20> POW10_64 = {
    1ULL,10ULL,100ULL,1000ULL,10000ULL,100000ULL,1000000ULL,10000000ULL,
    100000000ULL,1000000000ULL,10000000000ULL,100000000000ULL,
    1000000000000ULL,10000000000000ULL,100000000000000ULL,
    1000000000000000ULL,10000000000000000ULL,100000000000000000ULL,
    1000000000000000000ULL,10000000000000000000ULL
};

FD_ALWAYS_INLINE int digits128(u128 v) noexcept {
    if (v == 0) return 1;
    int lo = 1, hi = 38;
    while (lo < hi) {
        int mid = (lo + hi + 1) >> 1;
        if (v >= POW10.v[mid]) lo = mid; else hi = mid - 1;
    }
    return lo + 1;
}

FD_ALWAYS_INLINE int digits64(u64 v) noexcept {
    if (v < 10ULL)                  return 1;
    if (v < 100ULL)                 return 2;
    if (v < 1000ULL)                return 3;
    if (v < 10000ULL)               return 4;
    if (v < 100000ULL)              return 5;
    if (v < 1000000ULL)             return 6;
    if (v < 10000000ULL)            return 7;
    if (v < 100000000ULL)           return 8;
    if (v < 1000000000ULL)          return 9;
    if (v < 10000000000ULL)         return 10;
    if (v < 100000000000ULL)        return 11;
    if (v < 1000000000000ULL)       return 12;
    if (v < 10000000000000ULL)      return 13;
    if (v < 100000000000000ULL)     return 14;
    if (v < 1000000000000000ULL)    return 15;
    if (v < 10000000000000000ULL)   return 16;
    if (v < 100000000000000000ULL)  return 17;
    if (v < 1000000000000000000ULL) return 18;
    return 19;
}

// Architecture-optimized 128 / 64 division
FD_ALWAYS_INLINE u128 div128_by_64(u128 num, u64 den, u128& rem) noexcept {
#if (defined(__x86_64__) || defined(_M_X64)) && (defined(__GNUC__) || defined(__clang__))
    u64 nh = (u64)(num >> 64);
    u64 nl = (u64)num;
    if (nh < den) {
        u64 r;
        u64 ql;
        __asm__("divq %[den]"
                : "=a"(ql), "=d"(r)
                : [den]"r"(den), "a"(nl), "d"(nh));
        rem = r;
        return ql;
    } else {
        u64 qh = nh / den;
        u64 rh = nh % den;
        u64 ql, r;
        __asm__("divq %[den]"
                : "=a"(ql), "=d"(r)
                : [den]"r"(den), "a"(nl), "d"(rh));
        rem = r;
        return ((u128)qh << 64) | ql;
    }
#else
    // ARM64 / Generic 64-bit multi-precision fast divide
    u64 nh = (u64)(num >> 64);
    u64 nl = (u64)num;
    u64 qh = nh / den;
    u64 rh = nh % den;
    // (rh * 2^64 + nl) / den
    u128 rem_mid = ((u128)rh << 64) | nl;
    u128 ql = rem_mid / den;
    rem = rem_mid % den;
    return ((u128)qh << 64) | ql;
#endif
}

FD_ALWAYS_INLINE int u64_to_dec_lut(u64 v, char* dst, bool pad18) noexcept {
    char buf[20];
    int idx = 20;
    while (v >= 100) {
        u64 q = v / 100;
        u32 r = (u32)(v - q * 100);
        idx -= 2;
        buf[idx] = DIGIT_PAIRS[r * 2];
        buf[idx + 1] = DIGIT_PAIRS[r * 2 + 1];
        v = q;
    }
    if (v >= 10) {
        idx -= 2;
        buf[idx] = DIGIT_PAIRS[v * 2];
        buf[idx + 1] = DIGIT_PAIRS[v * 2 + 1];
    } else if (v > 0 || !pad18) {
        idx -= 1;
        buf[idx] = (char)('0' + v);
    }
    if (pad18) {
        int written = 20 - idx;
        int need_zeros = 18 - written;
        for (int i = 0; i < need_zeros; ++i) *dst++ = '0';
        std::memcpy(dst, buf + idx, written);
        return 18;
    } else {
        int written = 20 - idx;
        if (written == 0) { *dst = '0'; return 1; }
        std::memcpy(dst, buf + idx, written);
        return written;
    }
}

FD_ALWAYS_INLINE int u128_to_dec(u128 v, char* FD_RESTRICT buf) noexcept {
    if (v == 0) { buf[0] = '0'; return 1; }
    constexpr u64 E18 = 1000000000000000000ULL;

    u128 hi = v / E18;
    u64 g0 = (u64)(v - hi * E18);
    u64 g1 = (u64)(hi % E18);
    u64 g2 = (u64)(hi / E18);

    char* ptr = buf;
    if (g2 > 0) {
        ptr += u64_to_dec_lut(g2, ptr, false);
        ptr += u64_to_dec_lut(g1, ptr, true);
        ptr += u64_to_dec_lut(g0, ptr, true);
    } else if (g1 > 0) {
        ptr += u64_to_dec_lut(g1, ptr, false);
        ptr += u64_to_dec_lut(g0, ptr, true);
    } else {
        ptr += u64_to_dec_lut(g0, ptr, false);
    }
    return (int)(ptr - buf);
}

// ===========================================================================
// Decimal — core type
// ===========================================================================
struct alignas(16) Decimal {
    u128  sig  = 0;
    i16   exp  = 0;
    i8    sign = 0;
    u8    tag  = 0;

    Decimal() = default;

    FD_ALWAYS_INLINE explicit Decimal(i64 v) noexcept : exp(0), tag(0) {
        if (v < 0) { sign = 1; sig = (u128)(u64)(-(v + 1)) + 1; }
        else        { sign = 0; sig = (u128)(u64)v; }
    }
    FD_ALWAYS_INLINE explicit Decimal(u64 v) noexcept : sig(v), exp(0), sign(0), tag(0) {}
    FD_ALWAYS_INLINE explicit Decimal(double v) noexcept { from_double(v); }

    FD_ALWAYS_INLINE static Decimal raw(u128 s, i16 e, i8 sg) noexcept {
        Decimal d; d.sig = s; d.exp = e; d.sign = sg; d.tag = 0; return d;
    }
    FD_ALWAYS_INLINE static Decimal nan()      noexcept { Decimal d{}; d.tag = 2; return d; }
    FD_ALWAYS_INLINE static Decimal inf(int s) noexcept { Decimal d{}; d.tag = 1; d.sign = (i8)s; return d; }

    FD_ALWAYS_INLINE bool is_nan()    const noexcept { return tag == 2; }
    FD_ALWAYS_INLINE bool is_inf()    const noexcept { return tag == 1; }
    FD_ALWAYS_INLINE bool is_finite() const noexcept { return tag == 0; }
    FD_ALWAYS_INLINE bool is_zero()   const noexcept { return tag == 0 && sig == 0; }
    FD_ALWAYS_INLINE bool is_neg()    const noexcept { return sign != 0 && !is_zero(); }
    FD_ALWAYS_INLINE bool is_pos()    const noexcept { return sign == 0 && !is_zero(); }

    FD_ALWAYS_INLINE static void align(Decimal& a, Decimal& b) noexcept {
        if (a.exp == b.exp) return;
        if (a.exp > b.exp) {
            int sh = a.exp - b.exp;
            if (sh > MAX_DIGITS) { b.sig = 0; return; }
            a.sig *= POW10.v[sh]; a.exp = b.exp;
        } else {
            int sh = b.exp - a.exp;
            if (sh > MAX_DIGITS) { a.sig = 0; return; }
            b.sig *= POW10.v[sh]; b.exp = a.exp;
        }
    }

    FD_ALWAYS_INLINE static int cmp_mag(const Decimal& a, const Decimal& b) noexcept {
        if (a.exp == b.exp) return (a.sig > b.sig) - (a.sig < b.sig);
        int diff = (int)a.exp - (int)b.exp;
        if (diff > 0) {
            if (diff > MAX_DIGITS) return a.sig ? 1 : (b.sig ? -1 : 0);
            u128 as = a.sig * POW10.v[diff];
            return (as > b.sig) - (as < b.sig);
        }
        diff = -diff;
        if (diff > MAX_DIGITS) return b.sig ? -1 : (a.sig ? 1 : 0);
        u128 bs = b.sig * POW10.v[diff];
        return (a.sig > bs) - (a.sig < bs);
    }

    FD_ALWAYS_INLINE friend Decimal operator+(Decimal a, Decimal b) noexcept { return add(a, b); }
    FD_ALWAYS_INLINE friend Decimal operator-(Decimal a, Decimal b) noexcept { b.sign ^= 1; return add(a, b); }
    FD_ALWAYS_INLINE friend Decimal operator*(const Decimal& a, const Decimal& b) noexcept { return mul(a, b); }
    FD_ALWAYS_INLINE friend Decimal operator/(const Decimal& a, const Decimal& b) noexcept { return div(a, b); }
    FD_ALWAYS_INLINE Decimal& operator+=(const Decimal& o) noexcept { *this = *this + o; return *this; }
    FD_ALWAYS_INLINE Decimal& operator-=(const Decimal& o) noexcept { *this = *this - o; return *this; }
    FD_ALWAYS_INLINE Decimal& operator*=(const Decimal& o) noexcept { *this = *this * o; return *this; }
    FD_ALWAYS_INLINE Decimal& operator/=(const Decimal& o) noexcept { *this = *this / o; return *this; }
    FD_ALWAYS_INLINE Decimal  operator-() const noexcept { Decimal r = *this; r.sign ^= 1; return r; }

    FD_ALWAYS_INLINE friend std::strong_ordering operator<=>(const Decimal& a, const Decimal& b) noexcept {
        if (FD_UNLIKELY(a.is_nan() || b.is_nan())) return std::strong_ordering::equivalent;
        if (a.is_zero() && b.is_zero()) return std::strong_ordering::equal;
        if (a.sign != b.sign) return a.sign ? std::strong_ordering::less : std::strong_ordering::greater;
        int c = cmp_mag(a, b); if (a.sign) c = -c;
        return c < 0 ? std::strong_ordering::less : c > 0 ? std::strong_ordering::greater : std::strong_ordering::equal;
    }
    FD_ALWAYS_INLINE friend bool operator==(const Decimal& a, const Decimal& b) noexcept { return (a <=> b) == std::strong_ordering::equal; }
    FD_ALWAYS_INLINE friend bool operator!=(const Decimal& a, const Decimal& b) noexcept { return !(a == b); }
    FD_ALWAYS_INLINE friend bool operator< (const Decimal& a, const Decimal& b) noexcept { return (a <=> b) == std::strong_ordering::less; }
    FD_ALWAYS_INLINE friend bool operator> (const Decimal& a, const Decimal& b) noexcept { return (a <=> b) == std::strong_ordering::greater; }
    FD_ALWAYS_INLINE friend bool operator<=(const Decimal& a, const Decimal& b) noexcept { return !(a > b); }
    FD_ALWAYS_INLINE friend bool operator>=(const Decimal& a, const Decimal& b) noexcept { return !(a < b); }

    FD_ALWAYS_INLINE Decimal abs() const noexcept { Decimal r = *this; r.sign = 0; return r; }
    Decimal sqrt()      const noexcept;
    Decimal pow(i64 n)  const noexcept;
    Decimal round(int places) const noexcept;
    Decimal floor()     const noexcept;
    Decimal ceil()      const noexcept;
    Decimal trunc()     const noexcept;

    FD_ALWAYS_INLINE i64    to_i64()    const noexcept;
    FD_ALWAYS_INLINE double to_double() const noexcept;
    std::string             to_string() const;

    static Decimal from_string(std::string_view sv) noexcept;
    static Decimal from_chars(const char* first, const char* last) noexcept;

private:
    void from_double(double v) noexcept;
    static Decimal add(Decimal a, Decimal b) noexcept;
    static Decimal mul(const Decimal& a, const Decimal& b) noexcept;
    static Decimal div(const Decimal& a, const Decimal& b) noexcept;
};

// ===========================================================================
// Core Operations
// ===========================================================================
FD_ALWAYS_INLINE Decimal Decimal::add(Decimal a, Decimal b) noexcept {
    if (FD_UNLIKELY(a.is_nan() || b.is_nan())) return nan();
    if (FD_UNLIKELY(a.is_inf() || b.is_inf())) {
        if (a.is_inf() && b.is_inf() && a.sign != b.sign) return nan();
        return a.is_inf() ? a : b;
    }
    if (a.is_zero()) return b;
    if (b.is_zero()) return a;

    align(a, b);
    Decimal r; r.exp = a.exp; r.tag = 0;
    if (a.sign == b.sign) {
        r.sig = a.sig + b.sig; r.sign = a.sign;
    } else if (a.sig >= b.sig) {
        r.sig = a.sig - b.sig; r.sign = a.sign;
    } else {
        r.sig = b.sig - a.sig; r.sign = b.sign;
    }
    if (FD_UNLIKELY(r.sig > SIG_MAX)) { r.sig = (r.sig + 5) / 10; r.exp++; }
    if (r.sig == 0) { r.sign = 0; return r; }
    while ((r.sig % 10) == 0) { r.sig /= 10; r.exp++; }
    return r;
}

FD_ALWAYS_INLINE u128 mul128hi(u128 a, u128 b) noexcept {
    u64 ah = (u64)(a >> 64), al = (u64)a, bh = (u64)(b >> 64), bl = (u64)b;
    u128 mid1 = (u128)ah * bl, mid2 = (u128)al * bh, lo = (u128)al * bl, hi = (u128)ah * bh;
    return hi + ((mid1 + mid2 + (lo >> 64)) >> 64);
}

FD_ALWAYS_INLINE Decimal Decimal::mul(const Decimal& a, const Decimal& b) noexcept {
    if (FD_UNLIKELY(a.is_nan() || b.is_nan())) return nan();
    if (FD_UNLIKELY(a.is_inf() || b.is_inf())) {
        if (a.is_zero() || b.is_zero()) return nan();
        return inf(a.sign ^ b.sign);
    }
    if (a.is_zero() || b.is_zero()) return raw(0, 0, 0);

    i8  rsign = a.sign ^ b.sign;
    i16 rexp = a.exp + b.exp;

    if (a.sig <= 9999999999999999999ULL && b.sig <= 9999999999999999999ULL) {
        u128 r = (u128)(u64)a.sig * (u64)b.sig;
        if (FD_UNLIKELY(r > SIG_MAX)) { r = (r + 5) / 10; rexp++; }
        while (r != 0 && (r % 10) == 0) { r /= 10; rexp++; }
        return raw(r, rexp, rsign);
    }

    u128 lo = a.sig * b.sig, hi = mul128hi(a.sig, b.sig);
    int extra = 0;
    if (hi == 0) {
        while (FD_UNLIKELY(lo > SIG_MAX)) { lo = (lo + 5) / 10; extra++; }
    } else {
        constexpr u128 INV10 = ((u128)1844674407370955161ULL << 64) | 11068046444225730969ULL;
        while (hi > 0) {
            u64 c = (u64)(hi % 10); hi /= 10;
            u128 ln = lo / 10 + (u128)c * INV10;
            ln += (c * 6 + (u64)(lo % 10)) / 10;
            lo = ln; extra++;
        }
        while (FD_UNLIKELY(lo > SIG_MAX)) { lo = (lo + 5) / 10; extra++; }
    }
    rexp += extra;
    while (lo != 0 && (lo % 10) == 0) { lo /= 10; rexp++; }
    return raw(lo, rexp, rsign);
}

FD_ALWAYS_INLINE Decimal Decimal::div(const Decimal& a, const Decimal& b) noexcept {
    if (FD_UNLIKELY(a.is_nan() || b.is_nan())) return nan();
    if (b.is_zero()) return a.is_zero() ? nan() : inf(a.sign ^ b.sign);
    if (a.is_zero()) return raw(0, 0, 0);
    if (FD_UNLIKELY(a.is_inf())) return b.is_inf() ? nan() : inf(a.sign ^ b.sign);

    i8  rsign = a.sign ^ b.sign;
    i16 rexp = (i16)(a.exp - b.exp);

    if (b.sig <= 0xFFFFFFFFFFFFFFFFULL) {
        u64  b64 = (u64)b.sig;
        u128 num = a.sig;
        int  scale = 0;

        int da = digits128(num);
        int db = digits64(b64);
        int need = 38 + db - da;
        if (need < 0) need = 0;

        u128 room = SIG_MAX / num;
        int ms = digits128(room) - 1;
        if (need > ms) need = ms;
        if (need > 38) need = 38;

        num *= POW10.v[need];
        scale = need;
        if (num < (u128)b64 && num <= SIG_MAX / 10) { num *= 10; scale++; }

        u128 rem;
        u128 q = div128_by_64(num, b64, rem);

        if (rem * 2 >= (u128)b64) q++;
        rexp -= (i16)scale;
        while (FD_UNLIKELY(q > SIG_MAX)) { q = (q + 5) / 10; rexp++; }
        while (q != 0 && (q % 10) == 0) { q /= 10; rexp++; }
        return raw(q, rexp, rsign);
    }

    u128 num = a.sig; int scale = 0;
    {
        int need = 38 + digits128(b.sig) - digits128(num);
        if (need < 0) need = 0;
        u128 room = SIG_MAX / num; int ms = digits128(room) - 1; if (need > ms) need = ms;
        if (need > 38) need = 38;
        num *= POW10.v[need]; scale = need;
        if (num < b.sig && num <= SIG_MAX / 10) { num *= 10; scale++; }
    }
    u128 q = num / b.sig, rem = num - q * b.sig;
    if (rem * 2 >= b.sig) q++;
    rexp -= (i16)scale;
    while (FD_UNLIKELY(q > SIG_MAX)) { q = (q + 5) / 10; rexp++; }
    while (q != 0 && (q % 10) == 0) { q /= 10; rexp++; }
    return raw(q, rexp, rsign);
}

inline Decimal Decimal::sqrt() const noexcept {
    if (is_nan() || is_neg()) return nan();
    if (is_zero()) return raw(0, 0, 0);
    if (is_inf())  return inf(0);
    double ds = std::sqrt(to_double());
    if (!std::isfinite(ds) || ds <= 0) ds = 1.0;
    Decimal x(ds), two((i64)2), self = *this;
    for (int i = 0; i < 12; ++i) {
        Decimal xn = (x + self / x) / two;
        if (xn == x) break;
        x = xn;
    }
    return x;
}

inline Decimal Decimal::pow(i64 n) const noexcept {
    if (is_nan()) return nan();
    if (n == 0) return Decimal((i64)1);
    if (n < 0)  return (Decimal((i64)1) / *this).pow(-n);
    Decimal result((i64)1), base = *this;
    u64 e = (u64)n;
    while (e > 0) {
        if (e & 1) result *= base;
        base *= base;
        e >>= 1;
    }
    return result;
}

inline Decimal Decimal::round(int places) const noexcept {
    if (!is_finite() || exp >= -places) return *this;
    int sh = -places - exp; if (sh <= 0) return *this; if (sh > MAX_DIGITS) return raw(0, 0, 0);
    u128 p = POW10.v[sh], q = sig / p, r = sig - q * p, half = p / 2;
    if (r > half || (r == half && (q & 1))) q++;
    return raw(q, (i16)(-places), sign);
}

inline Decimal Decimal::trunc() const noexcept {
    if (!is_finite() || exp >= 0) return *this;
    int sh = -exp; if (sh > MAX_DIGITS) return raw(0, 0, 0);
    return raw(sig / POW10.v[sh], 0, sign);
}

inline Decimal Decimal::floor() const noexcept {
    Decimal t = trunc(); if (sign && t != *this) t -= Decimal((i64)1); return t;
}

inline Decimal Decimal::ceil() const noexcept {
    Decimal t = trunc(); if (!sign && t != *this) t += Decimal((i64)1); return t;
}

FD_ALWAYS_INLINE i64 Decimal::to_i64() const noexcept {
    if (!is_finite()) return 0;
    Decimal t = trunc(); u128 s = t.sig;
    if (t.exp > 0 && t.exp <= 18) s *= POW10_64[t.exp];
    i64 r = (i64)(u64)s;
    return sign ? -r : r;
}

FD_ALWAYS_INLINE double Decimal::to_double() const noexcept {
    if (is_nan()) return __builtin_nan("");
    if (is_inf()) return sign ? -__builtin_inf() : __builtin_inf();
    if (is_zero()) return 0.0;
    double hi = (double)(u64)(sig >> 64);
    double lo = (double)(u64)sig;
    double r = hi * 18446744073709551616.0 + lo;
    r *= std::pow(10.0, (double)exp);
    return sign ? -r : r;
}

inline std::string Decimal::to_string() const {
    if (is_nan()) return "NaN";
    if (is_inf()) return sign ? "-Inf" : "Inf";
    if (is_zero()) return "0";
    char sigbuf[48];
    int slen = u128_to_dec(sig, sigbuf);
    std::string out; out.reserve(slen + 16);
    if (sign) out += '-';
    int dot_pos = slen + exp;
    if (exp >= 0) {
        out.append(sigbuf, slen);
        out.append((size_t)exp, '0');
    } else if (dot_pos > 0) {
        out.append(sigbuf, dot_pos);
        out += '.';
        out.append(sigbuf + dot_pos, slen - dot_pos);
    } else {
        out += "0.";
        out.append((size_t)(-dot_pos), '0');
        out.append(sigbuf, slen);
    }
    return out;
}

inline void Decimal::from_double(double v) noexcept {
    tag = 0;
    if (std::isnan(v)) { tag = 2; return; }
    if (std::isinf(v)) { tag = 1; sign = (v < 0) ? 1 : 0; return; }
    if (v == 0.0) { sig = 0; exp = 0; sign = 0; return; }
    sign = (v < 0) ? 1 : 0; if (v < 0) v = -v;

    u64 bits = std::bit_cast<u64>(v);
    int e2 = (int)((bits >> 52) & 0x7FF) - 1023 - 52;
    u64 m = (bits & ((1ULL << 52) - 1));
    if ((bits >> 52) & 0x7FF) m |= (1ULL << 52);
    else e2++;

    sig = (u128)m; this->exp = 0;
    if (e2 >= 0) {
        int sh = e2;
        while (sh > 0) {
            int s = sh > 60 ? 60 : sh; sh -= s;
            sig <<= s;
            while (sig > SIG_MAX) { sig = (sig + 5) / 10; this->exp++; }
        }
    } else {
        int k = -e2;
        this->exp = (i16)(-k);
        while (k > 0) {
            int step = k > 27 ? 27 : k; k -= step;
            u64 pow5 = 1;
            for (int i = 0; i < step; ++i) pow5 *= 5;
            sig *= (u128)pow5;
            while (sig > SIG_MAX) { sig = (sig + 5) / 10; this->exp++; }
        }
    }
    while (sig != 0 && (sig % 10) == 0) { sig /= 10; this->exp++; }
    while (sig > SIG_MAX) { sig = (sig + 5) / 10; this->exp++; }
}

inline Decimal Decimal::from_string(std::string_view sv) noexcept {
    return from_chars(sv.data(), sv.data() + sv.size());
}

inline Decimal Decimal::from_chars(const char* first, const char* last) noexcept {
    if (first == last) return nan();
    const char* p = first;
    i8 sign = 0;
    if (*p == '-') { sign = 1; ++p; } else if (*p == '+') { ++p; }
    if (last - p >= 3 && (p[0] | 32) == 'n' && (p[1] | 32) == 'a' && (p[2] | 32) == 'n') return nan();
    if (last - p >= 3 && (p[0] | 32) == 'i' && (p[1] | 32) == 'n' && (p[2] | 32) == 'f') return inf(sign);

    u128 sig = 0; i16 exp_adj = 0; int ndot = 0, ndig = 0; bool dot = false;

    while (p < last) {
        // Fast 8-byte SWAR chunk parser
        if (ndig + 8 <= MAX_DIGITS && last - p >= 8 && !dot) {
            uint64_t v;
            std::memcpy(&v, p, 8);
            uint64_t t1 = v - 0x3030303030303030ULL;
            uint64_t t2 = v + 0x4646464646464646ULL;
            if (((t1 | t2) & 0x8080808080808080ULL) == 0) {
                uint64_t be = __builtin_bswap64(t1);
                uint64_t mask16 = 0x00FF00FF00FF00FFULL;
                uint64_t step1 = ((be >> 8) & mask16) * 10 + (be & mask16);
                uint64_t mask32 = 0x0000FFFF0000FFFFULL;
                uint64_t step2 = ((step1 >> 16) & mask32) * 100 + (step1 & mask32);
                uint64_t chunk = (step2 >> 32) * 10000 + (u32)step2;

                sig = sig * 100000000ULL + chunk;
                ndig += 8;
                p += 8;
                continue;
            }
        }

        unsigned c = (unsigned char)*p - '0';
        if (c <= 9) {
            if (ndig < MAX_DIGITS) {
                sig = sig * 10 + c; ndig++;
                if (dot) ndot++;
            } else {
                if (!dot) exp_adj++;
            }
            ++p;
        } else if ((unsigned char)*p == '.' && !dot) {
            dot = true; ++p;
        } else if (((unsigned char)*p == 'e' || (unsigned char)*p == 'E') && p < last - 1) {
            ++p; int esign = 1;
            if (*p == '+') { ++p; } else if (*p == '-') { esign = -1; ++p; }
            int ev = 0;
            while (p < last && (unsigned char)(*p - '0') <= 9) { ev = ev * 10 + (*p - '0'); ++p; }
            exp_adj += (i16)(esign * ev); break;
        } else break;
    }
    i16 fexp = (i16)(exp_adj - ndot);
    while (sig != 0 && (sig % 10) == 0 && fexp < SCALE_MAX) { sig /= 10; fexp++; }
    return raw(sig, fexp, sign);
}

// ===========================================================================
// SIMD batch ops (AVX2 / AVX-512 for x86-64, NEON for ARM64)
// ===========================================================================
namespace simd {

// ── ARM NEON SIMD Implementation ───────────────────────────────────────────
#if defined(FD_HAS_NEON)

FD_ALWAYS_INLINE void add_i64_neon(const i64* FD_RESTRICT a, const i64* FD_RESTRICT b, i64* FD_RESTRICT dst, std::size_t n) noexcept {
    std::size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        int64x2_t va0 = vld1q_s64(a + i);
        int64x2_t vb0 = vld1q_s64(b + i);
        int64x2_t va1 = vld1q_s64(a + i + 2);
        int64x2_t vb1 = vld1q_s64(b + i + 2);
        vst1q_s64(dst + i,     vaddq_s64(va0, vb0));
        vst1q_s64(dst + i + 2, vaddq_s64(va1, vb1));
    }
    for (; i + 2 <= n; i += 2) {
        int64x2_t va = vld1q_s64(a + i);
        int64x2_t vb = vld1q_s64(b + i);
        vst1q_s64(dst + i, vaddq_s64(va, vb));
    }
    for (; i < n; ++i) dst[i] = a[i] + b[i];
}

FD_ALWAYS_INLINE i64 hsum_i64_neon(const i64* FD_RESTRICT a, std::size_t n) noexcept {
    int64x2_t acc0 = vdupq_n_s64(0);
    int64x2_t acc1 = vdupq_n_s64(0);
    std::size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        acc0 = vaddq_s64(acc0, vld1q_s64(a + i));
        acc1 = vaddq_s64(acc1, vld1q_s64(a + i + 2));
    }
    acc0 = vaddq_s64(acc0, acc1);
    for (; i + 2 <= n; i += 2) {
        acc0 = vaddq_s64(acc0, vld1q_s64(a + i));
    }
    i64 r = vgetq_lane_s64(acc0, 0) + vgetq_lane_s64(acc0, 1);
    for (; i < n; ++i) r += a[i];
    return r;
}

FD_ALWAYS_INLINE void cmp_gt_i64_neon(const i64* FD_RESTRICT a, const i64* FD_RESTRICT b, i8* FD_RESTRICT dst, std::size_t n) noexcept {
    std::size_t i = 0;
    for (; i + 2 <= n; i += 2) {
        int64x2_t va = vld1q_s64(a + i);
        int64x2_t vb = vld1q_s64(b + i);
        uint64x2_t cmp = vcgtq_s64(va, vb);
        dst[i + 0] = (i8)(vgetq_lane_u64(cmp, 0) ? 1 : 0);
        dst[i + 1] = (i8)(vgetq_lane_u64(cmp, 1) ? 1 : 0);
    }
    for (; i < n; ++i) dst[i] = (a[i] > b[i]) ? 1 : 0;
}

#endif // FD_HAS_NEON

// ── x86-64 AVX2 / AVX-512 SIMD Implementation ──────────────────────────────
#ifdef FD_HAS_AVX2
FD_ALWAYS_INLINE void add_i64_avx2(const i64* FD_RESTRICT a, const i64* FD_RESTRICT b, i64* FD_RESTRICT dst, std::size_t n) noexcept {
    std::size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        _mm256_storeu_si256((__m256i*)(dst + i),
            _mm256_add_epi64(_mm256_loadu_si256((__m256i*)(a + i)), _mm256_loadu_si256((__m256i*)(b + i))));
    }
    for (; i < n; ++i) dst[i] = a[i] + b[i];
}
FD_ALWAYS_INLINE i64 hsum_i64_avx2(const i64* FD_RESTRICT a, std::size_t n) noexcept {
    __m256i acc0 = _mm256_setzero_si256();
    __m256i acc1 = _mm256_setzero_si256();
    std::size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        acc0 = _mm256_add_epi64(acc0, _mm256_loadu_si256((__m256i*)(a + i)));
        acc1 = _mm256_add_epi64(acc1, _mm256_loadu_si256((__m256i*)(a + i + 4)));
    }
    acc0 = _mm256_add_epi64(acc0, acc1);
    for (; i + 4 <= n; i += 4) acc0 = _mm256_add_epi64(acc0, _mm256_loadu_si256((__m256i*)(a + i)));
    __m128i lo = _mm256_castsi256_si128(acc0), hi = _mm256_extracti128_si256(acc0, 1), s = _mm_add_epi64(lo, hi);
    i64 r = _mm_extract_epi64(s, 0) + _mm_extract_epi64(s, 1);
    for (; i < n; ++i) r += a[i];
    return r;
}
FD_ALWAYS_INLINE void cmp_gt_i64_avx2(const i64* FD_RESTRICT a, const i64* FD_RESTRICT b, i8* FD_RESTRICT dst, std::size_t n) noexcept {
    std::size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        __m256i cmp = _mm256_cmpgt_epi64(_mm256_loadu_si256((__m256i*)(a + i)), _mm256_loadu_si256((__m256i*)(b + i)));
        dst[i + 0] = (i8)(_mm256_extract_epi64(cmp, 0) & 1); dst[i + 1] = (i8)(_mm256_extract_epi64(cmp, 1) & 1);
        dst[i + 2] = (i8)(_mm256_extract_epi64(cmp, 2) & 1); dst[i + 3] = (i8)(_mm256_extract_epi64(cmp, 3) & 1);
    }
    for (; i < n; ++i) dst[i] = (a[i] > b[i]) ? 1 : 0;
}
#endif

#ifdef FD_HAS_AVX512
FD_ALWAYS_INLINE void add_i64_avx512(const i64* FD_RESTRICT a, const i64* FD_RESTRICT b, i64* FD_RESTRICT dst, std::size_t n) noexcept {
    std::size_t i = 0;
    for (; i + 8 <= n; i += 8) _mm512_storeu_si512(dst + i, _mm512_add_epi64(_mm512_loadu_si512(a + i), _mm512_loadu_si512(b + i)));
#ifdef FD_HAS_AVX2
    add_i64_avx2(a + i, b + i, dst + i, n - i);
#else
    for (; i < n; ++i) dst[i] = a[i] + b[i];
#endif
}
FD_ALWAYS_INLINE i64 hsum_i64_avx512(const i64* FD_RESTRICT a, std::size_t n) noexcept {
    __m512i acc = _mm512_setzero_si512(); std::size_t i = 0;
    for (; i + 8 <= n; i += 8) acc = _mm512_add_epi64(acc, _mm512_loadu_si512(a + i));
    i64 r = _mm512_reduce_add_epi64(acc);
    for (; i < n; ++i) r += a[i]; return r;
}
#endif

// ── Generic unified vector dispatch ─────────────────────────────────────────
FD_ALWAYS_INLINE void add_i64_batch(const i64* FD_RESTRICT a, const i64* FD_RESTRICT b, i64* FD_RESTRICT dst, std::size_t n) noexcept {
#if defined(FD_HAS_AVX512)
    add_i64_avx512(a, b, dst, n);
#elif defined(FD_HAS_AVX2)
    add_i64_avx2(a, b, dst, n);
#elif defined(FD_HAS_NEON)
    add_i64_neon(a, b, dst, n);
#else
    for (std::size_t i = 0; i < n; ++i) dst[i] = a[i] + b[i];
#endif
}

FD_ALWAYS_INLINE i64 hsum_i64_batch(const i64* FD_RESTRICT a, std::size_t n) noexcept {
#if defined(FD_HAS_AVX512)
    return hsum_i64_avx512(a, n);
#elif defined(FD_HAS_AVX2)
    return hsum_i64_avx2(a, n);
#elif defined(FD_HAS_NEON)
    return hsum_i64_neon(a, n);
#else
    i64 r = 0;
    for (std::size_t i = 0; i < n; ++i) r += a[i];
    return r;
#endif
}

} // namespace simd

// ===========================================================================
// FixedDecimal<Scale>
// ===========================================================================
template<int Scale>
struct FixedDecimal {
    static_assert(Scale >= 0 && Scale <= 18);
    static constexpr i64 FACTOR = (i64)POW10_64[Scale];
    i64 raw_val = 0;

    constexpr FixedDecimal() noexcept = default;
    constexpr explicit FixedDecimal(i64 raw) noexcept : raw_val(raw) {}
    FD_ALWAYS_INLINE static FixedDecimal from_int(i64 v) noexcept { return FixedDecimal(v * FACTOR); }
    FD_ALWAYS_INLINE static FixedDecimal from_double(double v) noexcept { return FixedDecimal((i64)(v * FACTOR + (v >= 0 ? 0.5 : -0.5))); }

    FD_ALWAYS_INLINE FixedDecimal operator+(FixedDecimal o) const noexcept { return FixedDecimal(raw_val + o.raw_val); }
    FD_ALWAYS_INLINE FixedDecimal operator-(FixedDecimal o) const noexcept { return FixedDecimal(raw_val - o.raw_val); }
    FD_ALWAYS_INLINE FixedDecimal operator-()               const noexcept { return FixedDecimal(-raw_val); }
    FD_ALWAYS_INLINE FixedDecimal operator*(FixedDecimal o) const noexcept { return FixedDecimal((i64)(((i128)raw_val * o.raw_val) / FACTOR)); }
    FD_ALWAYS_INLINE FixedDecimal operator/(FixedDecimal o) const noexcept { return FixedDecimal((i64)(((i128)raw_val * FACTOR) / o.raw_val)); }
    FD_ALWAYS_INLINE FixedDecimal& operator+=(FixedDecimal o) noexcept { raw_val += o.raw_val; return *this; }
    FD_ALWAYS_INLINE FixedDecimal& operator-=(FixedDecimal o) noexcept { raw_val -= o.raw_val; return *this; }
    FD_ALWAYS_INLINE FixedDecimal& operator*=(FixedDecimal o) noexcept { *this = *this * o; return *this; }
    FD_ALWAYS_INLINE FixedDecimal& operator/=(FixedDecimal o) noexcept { *this = *this / o; return *this; }
    FD_ALWAYS_INLINE auto operator<=>(const FixedDecimal&) const noexcept = default;
    FD_ALWAYS_INLINE bool operator==(const FixedDecimal&)  const noexcept = default;

    FD_ALWAYS_INLINE double  to_double()  const noexcept { return (double)raw_val / FACTOR; }
    FD_ALWAYS_INLINE i64     to_int()     const noexcept { return raw_val / FACTOR; }
    FD_ALWAYS_INLINE Decimal to_decimal() const noexcept {
        return Decimal::raw((u128)(raw_val < 0 ? -raw_val : raw_val), -(i16)Scale, raw_val < 0 ? 1 : 0);
    }
    std::string to_string() const {
        char buf[32]; i64 v = raw_val; int neg = 0;
        if (v < 0) { neg = 1; v = -v; }
        i64 ip = v / FACTOR, fp = v % FACTOR;
        if constexpr (Scale == 0) { auto [p, ec] = std::to_chars(buf, buf + 32, neg ? -ip : ip); return std::string(buf, p); }
        else {
            char* p = buf; if (neg) *p++ = '-';
            auto [ep, ec] = std::to_chars(p, buf + 32, ip); p = ep; *p++ = '.';
            char frac[24]; i64 ff = fp;
            for (int i = 0; i < Scale; ++i) { frac[Scale - 1 - i] = '0' + (int)(ff % 10); ff /= 10; }
            std::memcpy(p, frac, Scale); p += Scale;
            return std::string(buf, p);
        }
    }
};

using Money2   = FixedDecimal<2>;
using Money4   = FixedDecimal<4>;
using Money8   = FixedDecimal<8>;
using Decimal6 = FixedDecimal<6>;
using Decimal9 = FixedDecimal<9>;

} // namespace fd
