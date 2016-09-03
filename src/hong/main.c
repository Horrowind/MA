#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include "database.h"
#include "pool.h"
#include "stack.h"

typedef enum thread_state {
    THREAD_STATE_TERMINATING,
    THREAD_STATE_WAITING,
    THREAD_STATE_RUNNING,
    THREAD_STATE_CAN_SHARE,
} thread_state_t;

typedef struct {
    thrd_t thread_id;
    cnd_t thread_cond;
    page_allocator_t page_allocator;
    queue_t queues[MAX_SIZE];
    enum thread_state state;
} thread_data_t;

typedef struct {
    int number_of_active_threads;
    thread_data_t thread_data[NUM_THREADS];
    mtx_t mutex;
    database_t database;
} thread_manager_t;

    
void thread_manager_init(thread_manager_t* thread_manager, database_t database) {
    thread_manager->number_of_active_threads = NUM_THREADS;
    thread_manager->database = database;
    mtx_init(&thread_manager->mutex, mtx_plain);
    for(int thread = 0; thread < NUM_THREADS; thread++) { 
        page_allocator_init(&thread_manager->thread_data[thread].page_allocator);
        for(int size = 0; size < MAX_SIZE; size++) {
            queue_init(&thread_manager->thread_data[thread].queues[size], &thread_manager->thread_data[thread].page_allocator);
        }
        thread_manager->thread_data[thread].state = THREAD_STATE_RUNNING;
        cnd_init(&thread_manager->thread_data[thread].thread_cond);
    }
}

void thread_try_sharing(thread_manager_t* thread_manager, long thread_number) {
    thread_data_t* data = &thread_manager->thread_data[thread_number];
    int res = mtx_trylock(&thread_manager->mutex);
    if(res == 0) {
        if(thread_manager->number_of_active_threads < NUM_THREADS) {
            ulong next_waiting_thread = (thread_number + 1) % NUM_THREADS;
            while(thread_manager->thread_data[next_waiting_thread].state != THREAD_STATE_WAITING) {
                next_waiting_thread = (next_waiting_thread + 1) % NUM_THREADS;
            }

            thread_data_t* other_data = &thread_manager->thread_data[next_waiting_thread];
                    
            for(int i = 1; i < MAX_SIZE; i++) {
                if(data->queues[i].head_page != data->queues[i].tail_page) {
                    queue_move_first_page(&data->queues[i], &other_data->queues[i]);
                }
            }
            other_data->state = THREAD_STATE_RUNNING;
            thread_manager->number_of_active_threads++;

            cnd_signal(&other_data->thread_cond);
        }
        mtx_unlock(&thread_manager->mutex);
    } else {
        assert(res == thrd_busy && "Error when trying to lock the mutex");
    }
}
    
void thread_wait(thread_manager_t* thread_manager, ulong thread_number) {
    thread_data_t* data = &thread_manager->thread_data[thread_number];
    mtx_lock(&thread_manager->mutex);
    thread_manager->number_of_active_threads--;
    if(thread_manager->number_of_active_threads == 0) {
        for(long other_thread = (thread_number + 1) % NUM_THREADS;
            other_thread != thread_number;
            other_thread = (other_thread + 1) % NUM_THREADS) {
            thread_manager->thread_data[other_thread].state = THREAD_STATE_TERMINATING;
            cnd_signal(&thread_manager->thread_data[other_thread].thread_cond);
        }
        mtx_unlock(&thread_manager->mutex);
        page_allocator_deinit(&data->page_allocator);
        thrd_exit(0);
    }
    data->state = THREAD_STATE_WAITING;

    while(data->state == THREAD_STATE_WAITING) {
        cnd_wait(&data->thread_cond, &thread_manager->mutex);
    }
    if(data->state == THREAD_STATE_TERMINATING) {
        mtx_unlock(&thread_manager->mutex);
        page_allocator_deinit(&data->page_allocator);
        thrd_exit(0);
    }
    mtx_unlock(&thread_manager->mutex);
}

typedef struct {
    int ngon;
    int rotation;
    boundary_t boundary;
} search_direction_t;

typedef struct {
    boundary_t boundary;
    int current_search_direction;
    search_direction_t directions[2 * MAX_SIZE];
} search_stack_entry_t;

generate_stack(search_stack, search_stack_entry_t);
search_stack_t search_stack;

void init_search_stack_entry(boundary_t boundary, search_stack_entry_t* entry) {
    entry->boundary = boundary;
    for(int i = 0; i < boundary.size; i++) {
        entry->directions[i].ngon = small_ngon;
        entry->directions[i].rotation = i;
        entry->directions[i].boundary = boundary_normalize((boundary_remove(boundary_rotl(boundary, i), small_ngon, 0)));
    }
    for(int i = 0; i < boundary.size; i++) {
        entry->directions[i + boundary.size].ngon = large_ngon;
        entry->directions[i + boundary.size].rotation = i;
        entry->directions[i + boundary.size].boundary =
            boundary_normalize((boundary_remove(boundary_rotl(boundary, i), large_ngon, 0)));
    }

    entry->current_search_direction = 0;

    for(int i = 0; i < 2 * boundary.size - 1; i++) {
        for(int j = i + 1; j < 2 * boundary.size; j++) {
            if(entry->directions[i].boundary.size > entry->directions[j].boundary.size) {
                search_direction_t tmp = entry->directions[i];
                entry->directions[i] = entry->directions[j];
                entry->directions[j] = tmp;
            }
        }
        // We can ignore boundaries with size 0.
        if(entry->directions[i].boundary.size == 0) {
            entry->current_search_direction++;
        }
    }
#ifndef NDEBUG
    printf("Entry ");
    boundary_write(entry->boundary);
    printf("\n");
    for(int i = entry->current_search_direction; i < 2 * boundary.size; i++) {
        printf("%5i ", i);
        boundary_write(entry->directions[i].boundary);
        printf("\n%75i\n%75i\n", entry->directions[i].rotation, entry->directions[i].ngon);
    }
#endif //NDEBUG
}

b32 search_backward_in_database(database_t database, boundary_t start, boundary_t goal) {
    start = boundary_normalize(start);
    search_stack_do_empty(&search_stack);
    search_stack_entry_t start_entry;
    init_search_stack_entry(start, &start_entry);
    search_stack_push(&search_stack, start_entry);
    while(1) {
#ifndef NDEBUG
        for(int i = 0; i < search_stack.fill; i++) {
            boundary_write(search_stack.data[i].boundary);
            printf("\n");
        }
        printf("\n");
#endif //NDEBUG
        search_stack_entry_t* entry = search_stack_top(&search_stack);
        boundary_t boundary = entry->boundary;
        if(boundary.bits == goal.bits && boundary.size == goal.size) {
            return 1;
        }
        if(entry->current_search_direction < 2 * entry->boundary.size) {
            boundary_t new_boundary = entry->directions[entry->current_search_direction].boundary;
            b32 already_found = 0;
            for(int i = 0; i < search_stack.fill; i++) {
                if(new_boundary.bits == search_stack.data[i].boundary.bits) {
                    already_found = 1;
                    break;
                }
            }

            if(!already_found) {
                if(new_boundary.size < MAX_SIZE &&
                   database_contains(database, new_boundary)) {
                    search_stack_entry_t new_entry;
                    init_search_stack_entry(new_boundary, &new_entry);
                    search_stack_push(&search_stack, new_entry);
                } else {
                    entry->current_search_direction++;
                }
            } else {
                entry->current_search_direction++;
            }
        } else {
            search_stack_pop(&search_stack);
            if(search_stack_is_empty(&search_stack)) {
                return 0;
            } else {
                search_stack_top(&search_stack)->current_search_direction++;
            }
        }
    }
}

/* b32 search_backward_towards_database(database_t database, boundary_t start, int start_size) { */
/*     boundary_t boundary = start; */
/*     search_stack_do_empty(&search_stack); */
/*     search_stack_push(&search_stack, (search_stack_entry_t){.ngon = small_ngon, .rotation = 0}); */
/*     while(1) { */
/* #ifndef NDEBUG */
/*         for(int i = 0; i < search_stack.fill; i++) { */
/*             printf("(%i,%i)", search_stack.data[i].ngon, search_stack.data[i].rotation); */
/*         } */
/*         boundary_write(boundary); */
/*         printf("\n"); */
/* #endif //NDEBUG */
/*         search_stack_entry_t* entry = search_stack_top(&search_stack); */
        
/*         if(boundary.size < MAX_SIZE && database_contains(database, boundary_normalize(boundary))) { */
/*             return 1; */
/*         } */
        
/*         if(entry->rotation < boundary.size) { */
/*             boundary_t new_boundary = boundary_remove(boundary, entry->ngon); */
/*             if(new_boundary.size > MAX_SIZE && new_boundary.size < MAX_SEARCH_SIZE) { */
/*                 boundary = new_boundary; */
/*                 search_stack_push(&search_stack, (search_stack_entry_t){.ngon = small_ngon, .rotation = 0}); */
/*             } else { */
/*                 entry->rotation++; */
/*                 boundary = boundary_rotl(boundary, 1); */
/*             } */
/*         } else if(entry->ngon == small_ngon) { */
/*             entry->rotation = 0; */
/*             entry->ngon = large_ngon; */
/*         } else { */
/*             search_stack_pop(&search_stack); */
/*             if(search_stack_is_empty(&search_stack)) { */
/*                 return 0; */
/*             } else { */
/*                 entry = search_stack_top(&search_stack); */
/*                 boundary = boundary_insert(boundary, entry->ngon); */
/*                 entry->rotation++; */
/*                 boundary = boundary_rotl(boundary, 1); */
/*             } */
/*         } */
/*     } */
/* } */



int working_thread_add(void* thread_manager_ptr) {
    thread_manager_t* thread_manager = (thread_manager_t*)thread_manager_ptr;
    thrd_t self_id = thrd_current();
    ulong thread_number = -1;
    for(int i = 0; i < NUM_THREADS; i++) {
        if(thread_manager->thread_data[i].thread_id == self_id) {
            thread_number = i;
            break;
        }
    }
    assert(thread_number != -1);
    thread_data_t* data = &thread_manager->thread_data[thread_number];

    while(1) {
        if(thread_manager->number_of_active_threads < NUM_THREADS && data->state == THREAD_STATE_CAN_SHARE) {
            thread_try_sharing(thread_manager, thread_number);
        }
        
        int has_no_items = 1;
        for(int size = 1; size < MAX_SIZE; size++) {
            if(!queue_is_empty(&data->queues[size])) {
                boundary_t boundary = {
                    .bits = queue_dequeue(&data->queues[size]),
                    .size = size
                };
                boundary_check(boundary);
                if(!database_contains(thread_manager->database, boundary)) {
#ifndef NDEBUG
                    printf("Thread %li found ", thread_number);
                    boundary_write(boundary);
                    printf("\n");
#endif
                    for(int j = 0; j < size; j++) {
                        boundary_t new_boundary = boundary_insert(boundary, small_ngon, 1);
                        if(new_boundary.size != 0 && new_boundary.size < MAX_SIZE) {
                            queue_enqueue(&data->queues[new_boundary.size], boundary_normalize(new_boundary).bits);
                        }
                        new_boundary = boundary_insert(boundary, large_ngon, 1);
                        if(new_boundary.size != 0 && new_boundary.size < MAX_SIZE) {
                            queue_enqueue(&data->queues[new_boundary.size], boundary_normalize(new_boundary).bits);
                        }
                        boundary = boundary_rotl(boundary, 1);
                    }
                    database_add(thread_manager->database, boundary);
                }
                has_no_items = 0;
                if(data->queues[size].head_page != data->queues[size].tail_page) data->state = THREAD_STATE_CAN_SHARE;
                break;
            }
            
        }
        
        if(has_no_items) {
            thread_wait(thread_manager, thread_number);
        }
    };
}


int main(int argc, char* argv[]) {

    if(argc == 3) {
        small_ngon = atoi(argv[1]);
        large_ngon = atoi(argv[2]);
    };

    database_t monogon_database;
    database_t small_ngon_database;

    database_init(&monogon_database);
    database_init(&small_ngon_database);

    thread_manager_t monogon_thread_manager;   
    thread_manager_t small_ngon_thread_manager;   

    thread_manager_init(&monogon_thread_manager, monogon_database);
    thread_manager_init(&small_ngon_thread_manager, small_ngon_database);

    queue_enqueue(&monogon_thread_manager.thread_data[0].queues[1], 0); // insert monogon
    queue_enqueue(&small_ngon_thread_manager.thread_data[0].queues[small_ngon], 0); // insert small_ngon

    for(long thread_number = 0; thread_number < NUM_THREADS; thread_number++) {
        thrd_create(&monogon_thread_manager.thread_data[thread_number].thread_id,
                       working_thread_add, (void*)&monogon_thread_manager);
    }
    mtx_unlock(&monogon_thread_manager.mutex);

    for(int thread_number = 0; thread_number < NUM_THREADS; thread_number++) {
        printf("Waiting for thread %i...\n", thread_number);
        thrd_join(monogon_thread_manager.thread_data[thread_number].thread_id, NULL);
        printf("Waiting for thread %i successfully!\n", thread_number);
    }

    for(long thread_number = 0; thread_number < NUM_THREADS; thread_number++) {
        thrd_create(&small_ngon_thread_manager.thread_data[thread_number].thread_id,
                       working_thread_add, (void*)&small_ngon_thread_manager);
    }


    mtx_unlock(&small_ngon_thread_manager.mutex);
    
    for(int thread_number = 0; thread_number < NUM_THREADS; thread_number++) {
        printf("Waiting for thread %i...\n", thread_number);
        thrd_join(small_ngon_thread_manager.thread_data[thread_number].thread_id, NULL);
        printf("Waiting for thread %i successfully!\n", thread_number);
    }

    mtx_destroy(&monogon_thread_manager.mutex);
    mtx_destroy(&small_ngon_thread_manager.mutex);

    
    pool_t mouse_pool;
    pool_init(&mouse_pool);
    boundary_t* mouse_data = (boundary_t*)mouse_pool.data;

    int count = 0;
    boundary_t boundary;
    for(boundary.size = 1; boundary.size < MAX_SIZE; boundary.size++) {
        for(boundary.bits = 0; boundary.bits < (1 << boundary.size); boundary.bits++) {
            if(database_contains(monogon_database, boundary)) {
                count++;
            }
        }
    }
    printf("Count: %i\n", count);

    
    search_stack_init(&search_stack, 64);

    {
        boundary_t boundary;
        for(boundary.size = 1; boundary.size * 6 < MAX_SIZE; boundary.size++) {
            for(boundary.bits = 0; boundary.bits < (1 << boundary.size); boundary.bits++) {
                if(database_contains(monogon_database, boundary)) {
                    for(int k = 0; k < boundary.size; k++) {
                        if(boundary_is_mouse(boundary)) {
                            boundary_t* new_mouse = (boundary_t*)pool_alloc(&mouse_pool, sizeof(boundary_t));
                            *new_mouse = boundary;
                        
                            printf("Found mouse ");
                            boundary_write(boundary);
                            printf("\n");
                            break;
                        
                        }
                        boundary = boundary_rotl(boundary, 1);
                    }
                }
            }
        }
    }
    for(int i = 0; i < mouse_pool.fill / sizeof(boundary_t); i++) {
        boundary_t mouse = mouse_data[i];
        if(mouse.size * 6 < MAX_SIZE) {
            boundary_t mouse_6_expanded = boundary_unfold(mouse, 6);
            printf("Expansion: ");
            boundary_write(mouse_6_expanded);
            if(database_contains(small_ngon_database, mouse_6_expanded)) {
                printf(" is found\n");
                boundary_t small_ngon_boundary = { .bits = 0, .size = small_ngon };
                b32 res = search_backward_in_database(small_ngon_database, mouse_6_expanded, small_ngon_boundary);
                if(res) {
                    printf("Found small_ngon replacement\n");
                    for(int i = 0; i < search_stack.fill; i++) {
                        boundary_write(search_stack.data[i].boundary);
                        printf("\n");
                    }
                    printf("\n");
                                    }
            } else {
                printf(" is not found\n");
            }
        }
    }

    pool_deinit(&mouse_pool);
    
    database_deinit(&monogon_database);
    database_deinit(&small_ngon_database);
    
    search_stack_deinit(&search_stack);
    
}
