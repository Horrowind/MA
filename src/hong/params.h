#ifndef PARAMS_H
#define PARAMS_H


#ifndef __OPTIMIZE__
#define inline
#endif

#define BOUNDARY_BITS 128
#define VALENCE        3
int small_ngon = 4;
int large_ngon = 9;
#define MAX_NGON_COUNT 2

#define NUM_THREADS 1
#define MAX_SIZE_CAP 26
#define MAX_SIZE ((BOUNDARY_BITS / 3) + 1 > MAX_SIZE_CAP ? MAX_SIZE_CAP : (BOUNDARY_BITS / 3) + 1)
#define MAX_SEARCH_SIZE 50

#define BITS           (VALENCE < 4 ? 1 : 2)
#define LAST_NODE_MASK ((1ull << BITS) - 1)


#endif //PARAMS_H
