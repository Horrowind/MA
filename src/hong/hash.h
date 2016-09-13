#ifndef BASE_HASH_H
#define BASE_HASH_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "datasizes.h"

#define MAX_FILL_NOMINATOR 9
#define MAX_FILL_DENOMINATOR 10
#define DELETED_HASH 0x00000000

#define STR2(X) #X
#define STR(X) STR2(X)

#define hashmap_t(type)                                                 \
    struct {                                                            \
        struct {                                                        \
            type data;                                                  \
            u32  hash;                                                  \
        }* entries;                                                     \
        uint size;                                                      \
        uint fill;                                                      \
    }

#define define_hashmap(name, type, fun, cmp)                            \
    typedef hashmap_t(type) name##_t;                                   \
    void name##_init(name##_t* map, int size);                          \
    void name##_deinit(name##_t* map);                                  \
    void name##_resize(name##_t* map);                                  \
    type* name##_find(name##_t* map, type data);                        \
    void name##_insert(name##_t* map, type data);                       \
    void name##_insert_unique(name##_t* map, type data);                \
    void name##_remove(name##_t* map, type data);                       \
    void name##_insert2(name##_t* map, type data, u32 hash);            \
    void name##_insert2_unique(name##_t* map, type data, u32 hash);     \
    void name##_check(name##_t* map);

#define implement_hashmap(name, type, fun, cmp)                         \
    void name##_init(name##_t* map, int size) {                         \
        assert( size && !(size & (size - 1)));                          \
        map->size = size;                                               \
        map->fill = 0;                                                  \
        uint num_bytes = size * sizeof(map->entries[0]);                \
        map->entries = malloc(num_bytes);                               \
        memset(map->entries, 0, num_bytes);                             \
    }                                                                   \
                                                                        \
    void name##_deinit(name##_t* map) {                                 \
        free(map->entries);                                             \
    }                                                                   \
                                                                        \
    void name##_insert2(name##_t* map, type data, u32 hash);            \
    void name##_insert2_unique(name##_t* map, type data, u32 hash);     \
    void name##_resize(name##_t* map) {                                 \
        name##_t new_map;                                               \
        name##_init(&new_map, 2 * map->size);                           \
        for(int i = 0; i < map->size; i++) {                            \
            type data = map->entries[i].data;                           \
            u32 hash = map->entries[i].hash;                            \
            if(hash != DELETED_HASH) {                                  \
                name##_insert2(&new_map, data, hash);                   \
            }                                                           \
        }                                                               \
        free(map->entries);                                             \
        *map = new_map;                                                 \
    }                                                                   \
                                                                        \
    inline                                                              \
    void name##_insert2_unique(name##_t* map, type data, u32 hash) {    \
        u32 mask = (map->size - 1);                                     \
        u32 pos = hash & mask;                                          \
        u32 dist = 0;                                                   \
        for(;;) {                                                       \
            if(map->entries[pos].hash == DELETED_HASH) {                \
                map->entries[pos].data = data;                          \
                map->entries[pos].hash = hash;                          \
                break;                                                  \
            } else {                                                    \
                if(hash == map->entries[pos].hash                       \
                   && cmp(map->entries[pos].data, data)) return;        \
                u32 probed_hash = map->entries[pos].hash;               \
                u32 probed_dist = (probed_hash - pos) & mask;           \
                if(probed_dist < dist) {                                \
                    type tmp_data = data;                               \
                    data = map->entries[pos].data;                      \
                    map->entries[pos].data = tmp_data;                  \
                                                                        \
                    u32 tmp_hash = hash;                                \
                    hash = probed_hash;                                 \
                    map->entries[pos].hash = tmp_hash;                  \
                }                                                       \
            }                                                           \
            pos = (pos + 1) & mask;                                     \
            dist++;                                                     \
        }                                                               \
        map->fill++;                                                    \
        if(map->fill * MAX_FILL_DENOMINATOR > map->size * MAX_FILL_NOMINATOR) \
            name##_resize(map);                                         \
    }                                                                   \
    inline                                                              \
    void name##_insert2(name##_t* map, type data, u32 hash) {           \
        u32 mask = (map->size - 1);                                     \
        u32 pos = hash & mask;                                          \
        u32 dist = 0;                                                   \
        for(;;) {                                                       \
            if(map->entries[pos].hash == DELETED_HASH) {                \
                map->entries[pos].data = data;                          \
                map->entries[pos].hash = hash;                          \
                break;                                                  \
            } else {                                                    \
                u32 probed_hash = map->entries[pos].hash;               \
                u32 probed_dist = (pos - probed_hash) & mask;           \
                /* printf("%i %i %i\n", probed_dist, map->fill, map->size);*/ \
                if(probed_dist < dist) {                                \
                    type tmp_data = data;                               \
                    data = map->entries[pos].data;                      \
                    map->entries[pos].data = tmp_data;                  \
                                                                        \
                    u32 tmp_hash = hash;                                \
                    hash = probed_hash;                                 \
                    map->entries[pos].hash = tmp_hash;                  \
                }                                                       \
            }                                                           \
            pos = (pos + 1) & mask;                                     \
            dist++;                                                     \
        }                                                               \
        map->fill++;                                                    \
        if(map->fill * MAX_FILL_DENOMINATOR > map->size * MAX_FILL_NOMINATOR) \
            name##_resize(map);                                         \
    }                                                                   \
                                                                        \
                                                                        \
    void name##_insert_unique(name##_t* map, type data) {               \
        u32 hash = fun(data);                                           \
        name##_insert2_unique(map, data, hash);                         \
    }                                                                   \
                                                                        \
    void name##_insert(name##_t* map, type data) {                      \
        u32 hash = fun(data);                                           \
        name##_insert2(map, data, hash);                                \
    }                                                                   \
                                                                        \
    type* name##_find(name##_t* map, type data) {                       \
        u32 hash = fun(data);                                           \
        u32 mask = (map->size - 1);                                     \
        u32 dist = 0;                                                   \
        u32 pos = hash & mask;                                          \
        for(;;) {                                                       \
            if(map->entries[pos].hash == DELETED_HASH) {                \
                return NULL;                                            \
            } else  if(cmp(map->entries[pos].data, data)) {             \
                return &map->entries[pos].data;                         \
            } else {                                                    \
                u32 probed_hash = map->entries[pos].hash;               \
                u32 probed_dist = (pos - probed_hash) & mask;           \
                if(probed_dist < dist) {                                \
                    return NULL;                                        \
                }                                                       \
                pos = (pos + 1) & mask;                                 \
                dist++;                                                 \
            }                                                           \
        }                                                               \
    }                                                                   \
    void name##_remove(name##_t* map, type data) {                      \
        u32 mask = (map->size - 1);                                     \
        type* deleted_record = name##_find(map, data);                  \
        if(deleted_record == NULL) return;                              \
        int pos = ((u8*)deleted_record - (u8*)map->entries) / sizeof(map->entries[0]); \
        for(;;) {                                                       \
            pos = (pos + 1) & mask;                                     \
            u32 probed_hash = map->entries[pos].hash;                   \
            u32 probed_dist = (pos - probed_hash) & mask;               \
            if(probed_hash == DELETED_HASH) {                           \
                break;                                                  \
            } else if(probed_dist == 0) {                               \
                break;                                                  \
            } else {                                                    \
                map->entries[(pos - 1) & mask] = map->entries[pos];     \
            }                                                           \
        }                                                               \
        map->entries[(pos - 1) & mask].hash = DELETED_HASH;             \
        map->fill--;                                                    \
    }                                                                   \
    void name##_check(name##_t* map) {                                  \
        int count = 0;                                                  \
        for(int i = 0; i < map->size; i++) {                            \
            if(map->entries[i].hash != DELETED_HASH) {                  \
                count++;                                                \
            }                                                           \
        }                                                               \
        assert(count == map->fill);                                     \
    }


#define generate_hashmap(name, type, fun, cmp)                          \
    define_hashmap(name, type, fun, cmp)                                \
    implement_hashmap(name, type, fun, cmp)


#endif
