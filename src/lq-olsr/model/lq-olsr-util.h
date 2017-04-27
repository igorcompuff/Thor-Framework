#ifndef LQ_OLSR_UTIL_H
#define LQ_OLSR_UTIL_H

#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))

#include <stdint.h>

uint64_t pack754(long double f, unsigned bits, unsigned expbits);

long double unpack754(uint64_t i, unsigned bits, unsigned expbits);

//uint64_t pack75432(float f);

//long double unpack75432(uint64_t i);

#endif
