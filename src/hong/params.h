#ifndef PARAMS_H
#define PARAMS_H


#ifndef __OPTIMIZE__
#define inline
#endif

#define BOUNDARY_BITS 128

#if BOUNDARY_BITS == 32
typedef uint32_t boundary_bits_t;
#else
#if BOUNDARY_BITS == 64
typedef uint64_t boundary_bits_t;
#else
#if BOUNDARY_BITS == 128
typedef __uint128_t boundary_bits_t;
#else
#error "BOUNDARY_BITS is neither 32, 64 nor 128!\n"
#endif
#endif
#endif


#define VALENCE  3
int small_ngon = 5;
int large_ngon = 7;
#define MAX_NGON_COUNT 2

#define NUM_THREADS 2
#define MAX_SIZE_CAP 30
#define MAX_SIZE ((BOUNDARY_BITS / 6 / BITS) + 1 > MAX_SIZE_CAP ? MAX_SIZE_CAP : (BOUNDARY_BITS / 6 / BITS) + 1)

#define MAX_SEARCH_SIZE 50

#define BITS           (VALENCE < 4 ? 1 : 2)
#define LAST_NODE_MASK (((boundary_bits_t)1 << BITS) - 1)


#endif //PARAMS_H
