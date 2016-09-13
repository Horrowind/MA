#ifndef QUEUE_H
#define QUEUE_H

#include "datasizes.h"
#include "boundary.h"

#define PAGE_SIZE 16384 * 4
#define QUEUE_SIZE_PER_PAGE ((PAGE_SIZE - sizeof(void*)) / sizeof(boundary_bits_t))


typedef struct {
    void* empty_list;
    int empty_list_size;
} page_allocator_t;

typedef struct queue_page {
    struct queue_page* next_page;
    boundary_bits_t entries[QUEUE_SIZE_PER_PAGE];
} queue_page_t;

typedef struct {
    queue_page_t* head_page;
    int head_index;
    queue_page_t* tail_page;
    int tail_index;
    page_allocator_t* page_allocator;
} queue_t;

void page_allocator_init(page_allocator_t* page_allocator);
void page_allocator_deinit(page_allocator_t* page_allocator);
void* allocate_page(page_allocator_t* page_allocator);
void  deallocate_page(page_allocator_t* page_allocator, void* page);


void  queue_init(queue_t* queue, page_allocator_t* page_allocator);
void  queue_deinit(queue_t* queue);
b32   queue_is_empty(queue_t* queue);
void  queue_enqueue(queue_t* queue, boundary_bits_t item);
boundary_bits_t queue_dequeue(queue_t* queue);
void  queue_move_first_page(queue_t* donor, queue_t* donee);



#endif //QUEUE_H
