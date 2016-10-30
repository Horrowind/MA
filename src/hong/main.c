#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "queue.h"
#include "database.h"
#include "pool.h"
#include "stack.h"
#include "hash.h"

#include "graphit.h"
#include "planar_graph.h"

typedef struct search_queue_entry {
    boundary_t boundary;
    struct search_queue_entry* prev;
    u8 ngon;
    u8 rotation;
} search_queue_entry_t;

#define SEARCH_QUEUE_SIZE_PER_PAGE ((PAGE_SIZE - sizeof(void*)) / sizeof(search_queue_entry_t))

typedef struct search_queue_page {
    struct search_queue_page* next_page;
    search_queue_entry_t entries[QUEUE_SIZE_PER_PAGE];
} search_queue_page_t;

typedef struct {
    search_queue_page_t* start_page;
    search_queue_page_t* search_page;
    int search_index;
    search_queue_page_t* insert_page;
    int insert_index;
    page_allocator_t* page_allocator;
} search_queue_t;

typedef struct {
    boundary_t boundary;
    uint path_length;
} search_hash_map_entry_t;

int search_hash_map_cmp(search_hash_map_entry_t entry1, search_hash_map_entry_t entry2) {
    return entry1.boundary.bits == entry2.boundary.bits;
}

u32 search_hash_map_hash(search_hash_map_entry_t entry) {
    /* u32 result = 0x10b85ada; */
    /* int primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67}; */
    /* int i = 0; */
    /* while(primes[i] < entry.boundary.size) { */
    /*     result ^= (u32)entry.boundary.bits; */
    /*     entry.boundary = boundary_rotl(entry.boundary, primes[i]); */
    /*     i++; */
    /* } */

    u32 result = 0x10b85ada ^ ((entry.boundary.bits ^ (entry.boundary.bits >> 16)) * 0x45d9f3b);
    result = (result ^ (result >> 16)) * 0x45d9f3b;
    result = result ^ (result >> 16);
    
    assert(result != DELETED_HASH);
    return result;
}

generate_hashmap(search_hash_map, search_hash_map_entry_t, search_hash_map_hash, search_hash_map_cmp);

void search_queue_init(search_queue_t* queue, page_allocator_t* page_allocator) {
    queue->page_allocator = page_allocator;
    queue->search_index = 0;
    queue->insert_index = 0;
    queue->start_page = queue->search_page = queue->insert_page = allocate_page(page_allocator);
}

void search_queue_deinit(search_queue_t* queue) {
    while(queue->start_page) {
        search_queue_page_t* page = queue->start_page;
        queue->start_page = queue->start_page->next_page;
        munmap(page, PAGE_SIZE); 
    }
}

inline
void search_queue_insert(search_queue_t* queue, search_queue_entry_t item) {
    search_queue_page_t* insert_page = queue->insert_page;
    if(queue->insert_index < SEARCH_QUEUE_SIZE_PER_PAGE) {
        insert_page->entries[queue->insert_index] = item;
        queue->insert_index++;
    } else {
        insert_page->next_page = (search_queue_page_t*)allocate_page(queue->page_allocator);
        /* queue_head_page->next_page->next_page = NULL; */
        queue->insert_page = insert_page->next_page; // !
        queue->insert_page->entries[0] = item;
        queue->insert_index = 1;
    }
}

inline
search_queue_entry_t* search_queue_get(search_queue_t* queue) {
    search_queue_page_t* search_page = queue->search_page;
    if(queue->search_index < SEARCH_QUEUE_SIZE_PER_PAGE) {
        return &search_page->entries[queue->search_index++];
    } else {
        search_page = search_page->next_page;
        queue->search_index = 1;
        return &search_page->entries[0];
    }
}

b32 search_queue_is_nonempty(search_queue_t* queue) {
    return queue->insert_page != queue->search_page || queue->search_index < queue->insert_index;
}

int system2(char* cmd) {
    int result = system(cmd);
    if (!WIFEXITED(result) && WIFSIGNALED(result)) exit(1);
    return result;
}


static inline
int heuristic(boundary_t boundary, boundary_t goal) {
    int min_diff = BOUNDARY_BITS / BITS;
    for(int i = 0; i < boundary.size; i++) {
        int diff = __builtin_popcountl(boundary_rotl(boundary, i).bits & goal.bits);
        if(diff < min_diff) min_diff = diff;
    }
    
    return boundary.size;
}

b32 search_database(boundary_t start, boundary_t goal, int* ngons, int ngons_count) {
    page_allocator_t page_allocator;
    page_allocator_init(&page_allocator);
    search_queue_t queues[512];

    boundary_t goal_normalized = boundary_normalize(goal);
    
    search_hash_map_t hash_map;
    search_hash_map_init(&hash_map, 1024);

    for(int i = 0; i < 512; i++) {
        search_queue_init(&queues[i], &page_allocator);
    }

    search_queue_insert(&queues[start.size], (search_queue_entry_t) {.boundary = start, .ngon = 0, .rotation = 0, .prev = NULL});
    int step = 0;
    while(1) {
        int nonempty_queue_index = -1;
        for(int i = 0; i < 512; i++) {
            if(search_queue_is_nonempty(&queues[i])) {
                nonempty_queue_index = i;
                break;
            }
        }
        if(nonempty_queue_index == -1) {
            printf("Empty! %i %i\n", start.size, step);
            for(int i = 0; i < 512; i++) {
                search_queue_deinit(&queues[i]);
            }
            return 0;
        }
        search_queue_entry_t* current_entry = search_queue_get(&queues[nonempty_queue_index]);
        boundary_t boundary = current_entry->boundary;
        boundary_t boundary_normalized = boundary_normalize(boundary);
        /* boundary_write(boundary); printf("\n"); */
        /* getchar(); */

        int stop = 1; // If set, search no more
	
        if(boundary_normalized.bits == goal_normalized.bits && boundary.size == goal.size) {
            planar_graph_builder_t builder;
            planar_graph_builder_from_boundary(&builder, current_entry->boundary);

            while(current_entry->prev != NULL) {
                planar_graph_builder_insert(&builder, current_entry->ngon, 0);
                planar_graph_builder_rotr(&builder, current_entry->rotation);
                /* planar_graph_builder_check_outer_edges(&builder); */
                current_entry = current_entry->prev;
            }


            planar_graph_builder_edge_t* edges = (planar_graph_builder_edge_t*)builder.edges_pool.data;
            int edge_count = builder.edges_pool.fill / sizeof(planar_graph_builder_edge_t);
            for(int i = 0; i < edge_count; i++) {
                if(edges[i].face1 == INNER_FACE || edges[i].face2 == INNER_FACE) continue;
                if(edges[i].face1 == OUTER_FACE || edges[i].face2 == OUTER_FACE) continue;
                for(int j = i + 1; j < edge_count; j++) {
                    // we have two faces, which are adjacent along two different edges
                    // => the graph is not 2-connected
                    if((edges[i].face1 == edges[j].face1 && edges[i].face2 == edges[j].face2) ||
                       (edges[i].face1 == edges[j].face2 && edges[i].face2 == edges[j].face1)) {
                        stop = 0;
                        break;
                    }
                }
            }

		

	    
            if(stop) {
                planar_graph_t planar_graph = planar_graph_from_builder(builder);

		int large_ngon_count = 0;
		for(int i = 1; i < planar_graph.face_count; i++) {
		    if(planar_graph.faces[i].ngon == large_ngon) {
			large_ngon_count++;
		    }
		}
		printf("large_ngon_count: %i\n", large_ngon_count);
		stop = planar_graph_output_sdl(planar_graph);
                planar_graph_deinit(planar_graph);
                if(stop) {
                    for(int i = 0; i < 512; i++) {
                        search_queue_deinit(&queues[i]);
                    }
                    pool_deinit(&builder.edges_pool);
                    return 2 - stop; // == 0 if stop == 2, and 1 if stop == 2
                }
            }
            pool_deinit(&builder.edges_pool);
            continue;
        }

        int path_length = nonempty_queue_index - heuristic(boundary, goal);
        /* if((step & 0xFFFFF) == 0) printf("Path length: %i %i %i \n", path_length, nonempty_queue_index, heuristic(boundary, goal)); */
        step++;
        if(step > 10000 * start.size) {
            for(int i = 0; i < 512; i++) {
                search_queue_deinit(&queues[i]);
            }
            return 0;
        }
        search_hash_map_entry_t entry = { .boundary = boundary_normalize(boundary), .path_length = path_length };
        search_hash_map_entry_t* found_entry = search_hash_map_find(&hash_map, entry);
        if(!found_entry || found_entry->path_length < path_length) {
            /* boundary_write(boundary); printf(" %i \n", path_length); */
            for(int i = 0; i < boundary.size; i++) {
                for(int j = 0; j < ngons_count; j++) {
                    boundary_t new_boundary = (boundary_remove(boundary_rotl(boundary, i), ngons[j], 0));
                    int heuristic_plus_path_length = heuristic(new_boundary, goal) + (path_length + 1);
                    assert(heuristic_plus_path_length < 512);
                    if(new_boundary.size > 0) {
//                        search_hash_map_entry_t new_entry = { .boundary = boundary_normalize(new_boundary) };
//                        if(!search_hash_map_find(&hash_map, new_entry)) {
                        search_queue_insert(&queues[heuristic_plus_path_length],
                                            (search_queue_entry_t) {
                                                .boundary = new_boundary,
                                                    .ngon = ngons[j],
                                                    .rotation = i,
                                                    .prev = current_entry
                                                    });
//                        }
                    }
                }
            }
            if(found_entry) {
                found_entry->path_length = path_length;
            } else {
                search_hash_map_insert(&hash_map, entry);
            }
        } else {
        }
    }
}


void test_paths() {
    database_t pentagon_database;
    database_init(&pentagon_database);
    boundary_t pentagon_boundary = { .size = 5, .bits = 0 };
    database_build_from_boundary(pentagon_database, pentagon_boundary, (int[]){ 5 }, 1);

    boundary_t test_boundary = {
        .bits = 0x3CF,
        .size = 12
    };
    test_boundary = boundary_normalize(test_boundary);
    search_database(test_boundary, pentagon_boundary, (int[]){ 5 }, 1);
    
    for(int size = 1; size < (MAX_SIZE - 12) / 2; size++) {
        printf("%i\n", size);
        for(int i = 0; i < (1 << size); i++) {
            test_boundary.size = 12 + 2 * size;
            boundary_bits_t rev = 0;
            for(int j = 0; j < size; j++) {
                rev |= ((i >> (size - j - 1)) & 1) << j;
            }
            test_boundary.bits = (((i << 6) | 0x1E) << (size + 6)) | (((~rev) & ((1 << size) - 1)) << 6) | 0x1E;
            //boundary_write(test_boundary); printf("\n");
            test_boundary = boundary_normalize(test_boundary);
            if(database_contains(pentagon_database, test_boundary)) {
                search_database(test_boundary, pentagon_boundary, (int[]){ 5 }, 1);
            }
        }
    }
}


int main(int argc, char* argv[]) {
    
    if(argc == 3) {
        small_ngon = atoi(argv[1]);
        large_ngon = atoi(argv[2]);
    };

    int ngons[] = { small_ngon, large_ngon };
    
    database_t monogon_database;
    /* database_t small_ngon_database; */

    database_init(&monogon_database);
    /* database_init(&small_ngon_database); */

    boundary_t small_ngon_boundary = { .size = small_ngon, .bits = 0 };

    boundary_t mouse_goal_boundary;
    switch(VALENCE) {
    case 3:
        mouse_goal_boundary.size = large_ngon - 3;
        mouse_goal_boundary.bits = (boundary_bits_t)1;
        break;
    case 4:
    case 5:
        mouse_goal_boundary.size = large_ngon - 1;
        mouse_goal_boundary.bits = (boundary_bits_t)2;
        break;
    }
    mouse_goal_boundary = boundary_normalize(mouse_goal_boundary);
    database_build_from_boundary(monogon_database, mouse_goal_boundary, ngons, 2);

    pool_t mouse_pool;
    pool_init(&mouse_pool);
    boundary_t* test_boundary = (boundary_t*)pool_alloc(&mouse_pool, sizeof(boundary_t));
    test_boundary->bits = 0x386D312C;
    test_boundary->size = 15;
    {
        boundary_t boundary;
        for(boundary.size = 1; boundary.size < MAX_SIZE; boundary.size++) {
            for(boundary.bits = 0; boundary.bits < (1 << (boundary.size * BITS)); boundary.bits++) {
                boundary_t boundary_normalized = boundary_normalize(boundary);
                if(database_contains(monogon_database, boundary_normalized)) {
                    for(int k = 0; k < boundary.size; k++) {
                        if(boundary_is_mouse(boundary)) {
                            boundary_t* mouses = (boundary_t*)mouse_pool.data;
                            int mouses_count = mouse_pool.fill / sizeof(boundary_t);
                            
                            b32 is_found = 0;
                            for(int i = 0; i < mouses_count; i++) {
                                if(mouses[i].bits == boundary_normalized.bits) {
                                    is_found = 1;
                                    break;
                                }
                            }
                            if(!is_found) {
                                boundary_t* new_mouse = (boundary_t*)pool_alloc(&mouse_pool, sizeof(boundary_t));
                                *new_mouse = boundary_normalize(boundary);
                                boundary_write(*new_mouse); printf("\n");
                            }
                        }
                        boundary = boundary_rotl(boundary, 1);
                    }
                }
            }
        }
    }

    database_deinit(&monogon_database);
    boundary_t* mouse_data = (boundary_t*)mouse_pool.data;
    int mouse_count = mouse_pool.fill / sizeof(boundary_t);
    printf("Found %i mouses\n", mouse_count);
    for(int i = 0; i < mouse_count; i++) {
        boundary_write(mouse_data[i]); printf("\n");
        b32 is_also_found = search_database(mouse_data[i], mouse_goal_boundary, ngons, 2);
        if(is_also_found) {
            int is_found;
            switch(VALENCE) {
            case 3:
                is_found = search_database(boundary_unfold(mouse_data[i], 6), small_ngon_boundary, ngons, 2);
                break;
            case 4:
                is_found = search_database(boundary_unfold(mouse_data[i], 4), small_ngon_boundary, ngons, 2);
                break;
            case 5:
                is_found = search_database(boundary_unfold(mouse_data[i], 5), small_ngon_boundary, ngons, 2);
                break;
            }
            if(is_found) {
                return 0;
            }
        }
    }    
}
