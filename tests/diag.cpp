#include "../include/decimal.hpp"
#include <cstdio>
using namespace fd;
static Decimal D(i64 v) { return Decimal(v); }
int main() {
    auto r = D(42) / D(6);
    printf("sig hi=%llu lo=%llu\n", (unsigned long long)(r.sig>>64), (unsigned long long)r.sig);
    printf("exp=%d sign=%d tag=%d\n", r.exp, r.sign, r.tag);
    printf("to_string=%s\n", r.to_string().c_str());
    printf("to_i64=%lld\n", (long long)r.to_i64());

    auto r2 = D(10) / D(4);
    printf("\n10/4: sig hi=%llu lo=%llu exp=%d\n", (unsigned long long)(r2.sig>>64), (unsigned long long)r2.sig, r2.exp);
    printf("10/4 to_string=%s\n", r2.to_string().c_str());
    printf("from_string('2.5')=%s\n", Decimal::from_string("2.5").to_string().c_str());

    auto r3 = D(1) * D(5) / D(10); // 0.5
    printf("\n1.5*4: sig=%llu exp=%d str=%s\n", (unsigned long long)r3.sig, r3.exp, r3.to_string().c_str());

    auto r4 = Decimal::from_string("1.5") * Decimal::from_string("4");
    printf("1.5*4: sig=%llu exp=%d str=%s\n", (unsigned long long)r4.sig, r4.exp, r4.to_string().c_str());
    return 0;
}
