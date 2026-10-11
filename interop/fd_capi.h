#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

// 32 bytes exact layout with 16-byte alignment
typedef struct {
    uint64_t sig_lo;
    uint64_t sig_hi;
    int16_t  exp;
    int8_t   sign;
    uint8_t  tag;
    uint8_t  pad[12];
} fd_dec_t;

// Pointer-based pass-by-reference / out-parameter functions for zero ABI mismatch & maximum FFI performance
void fd_from_i64(int64_t v, fd_dec_t* out);
void fd_from_u64(uint64_t v, fd_dec_t* out);
void fd_from_double(double v, fd_dec_t* out);
void fd_from_str(const char* s, size_t len, fd_dec_t* out);
void fd_nan(fd_dec_t* out);
void fd_inf(int negative, fd_dec_t* out);

void fd_add(const fd_dec_t* a, const fd_dec_t* b, fd_dec_t* out);
void fd_sub(const fd_dec_t* a, const fd_dec_t* b, fd_dec_t* out);
void fd_mul(const fd_dec_t* a, const fd_dec_t* b, fd_dec_t* out);
void fd_div(const fd_dec_t* a, const fd_dec_t* b, fd_dec_t* out);
void fd_neg(const fd_dec_t* a, fd_dec_t* out);
void fd_abs(const fd_dec_t* a, fd_dec_t* out);
void fd_sqrt(const fd_dec_t* a, fd_dec_t* out);
void fd_pow_i64(const fd_dec_t* a, int64_t n, fd_dec_t* out);

void fd_round(const fd_dec_t* a, int places, fd_dec_t* out);
void fd_trunc(const fd_dec_t* a, fd_dec_t* out);
void fd_floor(const fd_dec_t* a, fd_dec_t* out);
void fd_ceil(const fd_dec_t* a, fd_dec_t* out);

int  fd_cmp(const fd_dec_t* a, const fd_dec_t* b);
int  fd_eq(const fd_dec_t* a, const fd_dec_t* b);

int64_t fd_to_i64(const fd_dec_t* a);
double  fd_to_double(const fd_dec_t* a);
int     fd_to_str(const fd_dec_t* a, char* buf, size_t buflen);

int fd_is_nan(const fd_dec_t* a);
int fd_is_inf(const fd_dec_t* a);
int fd_is_zero(const fd_dec_t* a);
int fd_is_neg(const fd_dec_t* a);
int fd_is_finite(const fd_dec_t* a);

#ifdef __cplusplus
}
#endif
