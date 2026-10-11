#include <stdio.h>
#include "fd_capi.h"

int main() {
    printf("sizeof(fd_dec_t) = %zu\n", sizeof(fd_dec_t));
    fd_dec_t a = fd_from_str("123.456", 7);
    fd_dec_t b = fd_from_str("654.321", 7);
    fd_dec_t c = fd_add(a, b);
    char buf[64];
    fd_to_str(c, buf, 64);
    printf("c = %s\n", buf);
    return 0;
}
