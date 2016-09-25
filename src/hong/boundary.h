#ifndef BOUNDARY_H
#define BOUNDARY_H

#include <stdint.h>

#include "params.h"

#if BOUNDARY_BITS == 32
#define boundary_bits_ctz(x)                    \
    (__builtin_ctz(x))
#else

#if BOUNDARY_BITS == 64
#define boundary_bits_ctz(x)                    \
    (__builtin_ctzll(x))

#else
#if BOUNDARY_BITS == 128
static inline
int boundary_bits_ctz(boundary_bits_t v) {
    union _128_as_64 {
        __int128 v;
        uint64_t d[2];
    } u;
    u.v = v;
    if(u.d[0]) {
        return __builtin_ctzll(u.d[0]);
    } else {
        return __builtin_ctzll(u.d[1]);
    }
}

#else
#error "BOUNDARY_BITS is neither 32, 64 nor 128!\n"
#endif
#endif
#endif

#define boundary_bits_get_trailing_max_nodes(bits)			\
    (VALENCE != 4							\
     ? boundary_bits_ctz(~boundary.bits) / BITS				\
     : boundary_bits_ctz(~(boundary.bits | (((boundary_bits_t)-1) / ((boundary_bits_t)3)))) / BITS)

typedef struct  {
    boundary_bits_t bits;
    int size;
} boundary_t;

static
void boundary_write(boundary_t boundary);

static
void boundary_check(boundary_t boundary);

static inline
boundary_t boundary_normalize(boundary_t boundary);

#endif //BOUNDARY_H
