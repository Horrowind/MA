#include <assert.h>

#include <unistd.h>
#define _GNU_SOURCE
#include <sys/mman.h>
#include <sys/types.h>
#include <errno.h>

#include "database.h"
#include "queue.h"
#include "thread.h"

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
    u64 bits_used = (1ull << (MAX_SIZE * BITS)) + (MAX_SIZE * BITS);
    u64 pages_used = div_ceil(bits_used, 8 * page_size);
    database->entries = mmap(NULL, pages_used * page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    madvise(database->entries, pages_used * page_size, MADV_RANDOM);
    assert(database->entries != MAP_FAILED && "Allocation of Database failed\n");
}

static
void database_deinit(database_t* database) {
    u64 bits_used = (1ull << (MAX_SIZE * BITS)) + (MAX_SIZE * BITS);
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

typedef enum thread_state {
    THREAD_STATE_TERMINATING,
    THREAD_STATE_WAITING,
    THREAD_STATE_RUNNING,
    THREAD_STATE_CAN_SHARE,
} thread_state_t;

typedef struct {
    thread_t thread_id;
    cond_t thread_cond;
    page_allocator_t page_allocator;
    queue_t queues[MAX_SIZE];
    enum thread_state state;
} thread_data_t;

typedef struct {
    database_t database;
    mutex_t mutex;
    int ngons[MAX_NGON_COUNT];
    int ngon_count;
    volatile int number_of_active_threads;
    thread_data_t thread_data[NUM_THREADS];
} thread_manager_t;

void thread_manager_init(thread_manager_t* thread_manager, database_t database, int* ngons, int ngon_count) {
    assert(ngon_count <= MAX_NGON_COUNT);
    assert(ngons);
    for(int i = 0; i < ngon_count; i++) {
        thread_manager->ngons[i] = ngons[i];
    }
    thread_manager->ngon_count = ngon_count;
    thread_manager->number_of_active_threads = NUM_THREADS;
    thread_manager->database = database;
    mutex_init(&thread_manager->mutex);
    for(int thread = 0; thread < NUM_THREADS; thread++) { 
        page_allocator_init(&thread_manager->thread_data[thread].page_allocator);
        for(int size = 0; size < MAX_SIZE; size++) {
            queue_init(&thread_manager->thread_data[thread].queues[size], &thread_manager->thread_data[thread].page_allocator);
        }
        thread_manager->thread_data[thread].state = THREAD_STATE_RUNNING;
        cond_init(&thread_manager->thread_data[thread].thread_cond);
    }
}

void thread_manager_deinit(thread_manager_t* thread_manager) {
    for(int thread = 0; thread < NUM_THREADS; thread++) { 
        /* cond_deinit(&thread_manager->thread_data[thread].thread_cond); */
        /* for(int size = 0; size < MAX_SIZE; size++) { */
        /*     queue_deinit(&thread_manager->thread_data[thread].queues[size]); */
        /* } */
        page_allocator_deinit(&thread_manager->thread_data[thread].page_allocator);
    }
    mutex_deinit(&thread_manager->mutex);
}

void thread_try_sharing(thread_manager_t* thread_manager, long thread_number) {
    thread_data_t* data = &thread_manager->thread_data[thread_number];
    thread_error_t  res = mutex_trylock(&thread_manager->mutex);
    if(res == THREAD_ERROR_NONE) {
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
            cond_signal(&other_data->thread_cond);
        }
        mutex_unlock(&thread_manager->mutex);
    } else {
        assert(res == THREAD_ERROR_BUSY && "Error when trying to lock the mutex");
    }
}
    
void thread_wait(thread_manager_t* thread_manager, ulong thread_number) {
    thread_data_t* data = &thread_manager->thread_data[thread_number];
    mutex_lock(&thread_manager->mutex);
    thread_manager->number_of_active_threads--;
    if(thread_manager->number_of_active_threads == 0) {
        for(long other_thread = (thread_number + 1) % NUM_THREADS;
            other_thread != thread_number;
            other_thread = (other_thread + 1) % NUM_THREADS) {
            thread_manager->thread_data[other_thread].state = THREAD_STATE_TERMINATING;
            cond_signal(&thread_manager->thread_data[other_thread].thread_cond);
        }
        mutex_unlock(&thread_manager->mutex);
        thread_exit();
    }
    data->state = THREAD_STATE_WAITING;

    while(data->state == THREAD_STATE_WAITING) {
        cond_wait(&data->thread_cond, &thread_manager->mutex);
    }
    if(data->state == THREAD_STATE_TERMINATING) {
        mutex_unlock(&thread_manager->mutex);
        thread_exit();
    }
    mutex_unlock(&thread_manager->mutex);
}

THREAD_SIG(working_thread_add, thread_manager_ptr) {
    thread_manager_t* thread_manager = (thread_manager_t*)thread_manager_ptr;
    thread_t self_id = thread_current();
    ulong thread_number = -1;
    for(int i = 0; i < NUM_THREADS; i++) {
        if(thread_equal(thread_manager->thread_data[i].thread_id, self_id)) {
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
#ifndef EARLY_OUT
                if(!database_contains(thread_manager->database, boundary)) {
#endif
#if 1
//                    if(boundary_is_mouse(boundary)) {
                        printf("Thread %li found ", thread_number);
                        boundary_write(boundary);
                        printf("\n");
//                    }
#endif
                    for(int j = 0; j < size; j++) {
                        for(int k = 0; k < thread_manager->ngon_count; k++) {
                            int ngon = thread_manager->ngons[k];
                            boundary_t new_boundary = boundary_insert(boundary, ngon, 0);
                            if(new_boundary.size != 0 && new_boundary.size < MAX_SIZE) {
#ifdef EARLY_OUT
                                if(!database_contains(thread_manager->database, new_boundary)) {
                                    database_add(thread_manager->database, new_boundary);
                                    queue_enqueue(&data->queues[new_boundary.size], boundary_normalize(new_boundary).bits);
                                }
#else 
                                queue_enqueue(&data->queues[new_boundary.size], boundary_normalize(new_boundary).bits);
#endif
                            }
                        }
                        boundary = boundary_rotl(boundary, 1);
                    }
#ifndef EARLY_OUT
                    database_add(thread_manager->database, boundary);
                }
#endif
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


void database_build_from_boundary(database_t database, boundary_t boundary, int* ngons, int ngon_count) {
    thread_manager_t thread_manager;   
    thread_manager_init(&thread_manager, database, ngons, ngon_count);
    queue_enqueue(&thread_manager.thread_data[0].queues[boundary.size], boundary.bits);
        for(long thread_number = 0; thread_number < NUM_THREADS; thread_number++) {
        thread_create(&thread_manager.thread_data[thread_number].thread_id,
                       working_thread_add, (void*)&thread_manager);
    }
    mutex_unlock(&thread_manager.mutex);
    for(int thread_number = 0; thread_number < NUM_THREADS; thread_number++) {
#ifdef NDEBUG
        thread_join(thread_manager.thread_data[thread_number].thread_id);
#else
        printf("Waiting for thread %i...\n", thread_number);
        thread_join(thread_manager.thread_data[thread_number].thread_id);
        printf("Waiting for thread %i successfully!\n", thread_number);
#endif
    }

    thread_manager_deinit(&thread_manager);

    
}
