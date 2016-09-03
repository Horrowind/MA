#include <assert.h>

#include <unistd.h>
#define _GNU_SOURCE
#include <sys/mman.h>
#include <sys/types.h>
#include <errno.h>

#include "database.h"

#ifdef OLD_LAYOUT
#include <stdlib.h>

static
void database_init(database_t* database) {
    for(int i = 1; i < MAX_SIZE; i++) {
        u64 size = (i < 3 ? 1 : 1ull << (i - 3)) * BITS;
        if(size < sysconf(_SC_PAGESIZE)) {
            database[i] = malloc(size);
            assert(database[i] != 0 && "Allocation of Database failed\n");
            memset(database[i], 0, size);
        } else {
            database[i] = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            assert(database[i] != MAP_FAILED && "Allocation of Database failed\n");
        }
    }
}

static
void database_deinit(database_t* database) {
    for(int i = 0; i < MAX_SIZE; i++) {
        if(database[i] > 0) {
            u64 size = (i < 3 ? 1 : 1ull << (i - 3)) * BITS;
            if(size < sysconf(_SC_PAGESIZE))  {
                free(database[i]);
            } else {
                munmap(database[i], size);
            }
        }
    }
}

static inline
b32 database_contains(database_t database, boundary_t boundary) {
    return (database->entries[boundary.size][boundary.bits >> 3] & (1ull << (boundary.bits & 7))) >> (boundary.bits & 7);
}

static inline
void database_add(database_t database, boundary_t boundary) {
    database->entries[boundary.size][boundary.bits >> 3] |= 1ull << (boundary.bits & 7);
}
#else

static
void database_init(database_t* database) {
    u64 page_size = sysconf(_SC_PAGESIZE);
    u64 bits_used = (1ull << MAX_SIZE) + MAX_SIZE;
    u64 pages_used = div_ceil(bits_used, 8 * page_size);
    database->entries = mmap(NULL, pages_used * page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(database->entries != MAP_FAILED && "Allocation of Database failed\n");
}

static
void database_deinit(database_t* database) {
    u64 bits_used = (1ull << MAX_SIZE) + MAX_SIZE;
    u64 pages_used = div_ceil(bits_used, 8 * sysconf(_SC_PAGESIZE));
    munmap(database->entries, pages_used);
}

static inline
b32 database_contains(database_t database, boundary_t boundary) {
    boundary_bits_t lookup =  boundary.bits + boundary.size;
    return (database.entries[lookup >> 3] >> (lookup & 7)) & 1;
}

static inline
void database_add(database_t database, boundary_t boundary) {
    boundary_bits_t lookup =  boundary.bits + boundary.size;
    database.entries[lookup >> 3] |= 1ull << (lookup & 7);
}
    


#endif //OLD_LAYOUT
