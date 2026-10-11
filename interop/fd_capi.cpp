#include "fd_capi.h"
#include "../include/decimal.hpp"
#include <cstring>

static inline fd::Decimal to_cpp(const fd_dec_t* a) {
    fd::Decimal d;
    d.sig = ((fd::u128)a->sig_hi << 64) | a->sig_lo;
    d.exp = a->exp;
    d.sign = a->sign;
    d.tag = a->tag;
    return d;
}

static inline void to_c(const fd::Decimal& d, fd_dec_t* out) {
    std::memset(out, 0, sizeof(fd_dec_t));
    out->sig_lo = (uint64_t)d.sig;
    out->sig_hi = (uint64_t)(d.sig >> 64);
    out->exp = d.exp;
    out->sign = d.sign;
    out->tag = d.tag;
}

extern "C" {

void fd_from_i64(int64_t v, fd_dec_t* out) { to_c(fd::Decimal(v), out); }
void fd_from_u64(uint64_t v, fd_dec_t* out) { to_c(fd::Decimal(v), out); }
void fd_from_double(double v, fd_dec_t* out) { to_c(fd::Decimal(v), out); }

void fd_from_str(const char* s, size_t len, fd_dec_t* out) {
    if (!s) { to_c(fd::Decimal::nan(), out); return; }
    if (len == 0) len = std::strlen(s);
    to_c(fd::Decimal::from_chars(s, s + len), out);
}

void fd_nan(fd_dec_t* out) { to_c(fd::Decimal::nan(), out); }
void fd_inf(int negative, fd_dec_t* out) { to_c(fd::Decimal::inf(negative ? 1 : 0), out); }

void fd_add(const fd_dec_t* a, const fd_dec_t* b, fd_dec_t* out) { to_c(to_cpp(a) + to_cpp(b), out); }
void fd_sub(const fd_dec_t* a, const fd_dec_t* b, fd_dec_t* out) { to_c(to_cpp(a) - to_cpp(b), out); }
void fd_mul(const fd_dec_t* a, const fd_dec_t* b, fd_dec_t* out) { to_c(to_cpp(a) * to_cpp(b), out); }
void fd_div(const fd_dec_t* a, const fd_dec_t* b, fd_dec_t* out) { to_c(to_cpp(a) / to_cpp(b), out); }
void fd_neg(const fd_dec_t* a, fd_dec_t* out) { to_c(-to_cpp(a), out); }
void fd_abs(const fd_dec_t* a, fd_dec_t* out) { to_c(to_cpp(a).abs(), out); }
void fd_sqrt(const fd_dec_t* a, fd_dec_t* out) { to_c(to_cpp(a).sqrt(), out); }
void fd_pow_i64(const fd_dec_t* a, int64_t n, fd_dec_t* out) { to_c(to_cpp(a).pow(n), out); }

void fd_round(const fd_dec_t* a, int places, fd_dec_t* out) { to_c(to_cpp(a).round(places), out); }
void fd_trunc(const fd_dec_t* a, fd_dec_t* out) { to_c(to_cpp(a).trunc(), out); }
void fd_floor(const fd_dec_t* a, fd_dec_t* out) { to_c(to_cpp(a).floor(), out); }
void fd_ceil(const fd_dec_t* a, fd_dec_t* out) { to_c(to_cpp(a).ceil(), out); }

int fd_cmp(const fd_dec_t* a, const fd_dec_t* b) {
    auto ord = to_cpp(a) <=> to_cpp(b);
    if (ord == std::strong_ordering::less) return -1;
    if (ord == std::strong_ordering::greater) return 1;
    return 0;
}

int fd_eq(const fd_dec_t* a, const fd_dec_t* b) { return to_cpp(a) == to_cpp(b) ? 1 : 0; }

int64_t fd_to_i64(const fd_dec_t* a) { return to_cpp(a).to_i64(); }
double  fd_to_double(const fd_dec_t* a) { return to_cpp(a).to_double(); }

int fd_to_str(const fd_dec_t* a, char* buf, size_t buflen) {
    if (!buf || buflen == 0) return 0;
    std::string s = to_cpp(a).to_string();
    size_t copy_len = s.size() < buflen - 1 ? s.size() : buflen - 1;
    std::memcpy(buf, s.data(), copy_len);
    buf[copy_len] = '\0';
    return (int)copy_len;
}

int fd_is_nan(const fd_dec_t* a)    { return to_cpp(a).is_nan() ? 1 : 0; }
int fd_is_inf(const fd_dec_t* a)    { return to_cpp(a).is_inf() ? 1 : 0; }
int fd_is_zero(const fd_dec_t* a)   { return to_cpp(a).is_zero() ? 1 : 0; }
int fd_is_neg(const fd_dec_t* a)    { return to_cpp(a).is_neg() ? 1 : 0; }
int fd_is_finite(const fd_dec_t* a) { return to_cpp(a).is_finite() ? 1 : 0; }

}
