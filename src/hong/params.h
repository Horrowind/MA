#ifndef PARAMS_H
#define PARAMS_H


#ifndef __OPTIMIZE__
#define inline
#endif

#define NUM_THREADS 2
int small_ngon = 5;
int large_ngon = 7;
#define MAX_SIZE 31
#define MAX_SEARCH_SIZE 50

#define VALENCE        3
#define BITS           (VALENCE < 4 ? 1 : 2)
#define LAST_NODE_MASK ((1ull << BITS) - 1)


#endif //PARAMS_H
