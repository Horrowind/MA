#ifndef DATABASE_H
#define DATABASE_H

#include "datasizes.h"
#include "params.h"
#include "boundary.h"

#define div_ceil(a, b) (((a) + (b) - 1) / (b))


#ifdef OLD_LAYOUT

typedef struct {
    u8* entries[MAX_SIZE];
} database_t;

#else
typedef struct {
    u8* entries;
} database_t;

#endif //OLD_LAYOUT

static void database_init(database_t* database);
static void database_deinit(database_t* database);
static inline b32 database_contains(database_t database, boundary_t boundary);
static inline void database_add(database_t database, boundary_t boundary);


#endif //DATABASE_H
