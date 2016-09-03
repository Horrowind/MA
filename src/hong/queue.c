#include <sys/mman.h>

#include "queue.h"

#ifndef NULL
#define NULL ((void*)0)
#endif //NULL

void page_allocator_init(page_allocator_t* page_allocator) {
    page_allocator->empty_list = NULL;
}

void* allocate_page(page_allocator_t* page_allocator) {
    void* new_page;
    if(page_allocator->empty_list == NULL) {
        new_page = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    } else {
        new_page = page_allocator->empty_list;
        page_allocator->empty_list = *(void**)(new_page);
    }
    return new_page;
}

void deallocate_page(page_allocator_t* page_allocator, void* page) {
    void* old_empty_list_head = page_allocator->empty_list;
    *(void**)(page_allocator) = page;
    *(void**)(page) = old_empty_list_head;
}

void page_allocator_deinit(page_allocator_t* page_allocator) {
    while(page_allocator->empty_list != NULL) {
        void** entry_to_delete = page_allocator->empty_list;
        page_allocator->empty_list = *(void**)(page_allocator->empty_list);
        munmap(entry_to_delete, PAGE_SIZE); 
    }
}


void queue_init(queue_t* queue, page_allocator_t* page_allocator) {
    queue->page_allocator = page_allocator;
    queue->head_page = queue->tail_page = allocate_page(page_allocator);
    queue->head_index = queue->tail_index = 0;
}

void queue_deinit(queue_t* queue) {
    while(1) {
        queue_page_t* next_page = queue->head_page->next_page;
        deallocate_page(queue->page_allocator, queue->head_page);
        if(queue->head_page != queue->tail_page) break;
        queue->head_page = next_page;
    }
}


inline
void queue_enqueue(queue_t* queue, boundary_bits_t item) {
    queue_page_t* queue_head_page = queue->head_page;
    if(queue->head_index < QUEUE_SIZE_PER_PAGE) {
        queue_head_page->entries[queue->head_index] = item;
        queue->head_index++;
    } else {
        queue_head_page->next_page = (queue_page_t*)allocate_page(queue->page_allocator);
        /* queue_head_page->next_page->next_page = NULL; */
        queue->head_page = queue_head_page->next_page; // !
        queue->head_page->entries[0] = item;
        queue->head_index = 1;
    }
}
inline
b32 queue_is_empty(queue_t* queue) {
    return (queue->head_page == queue->tail_page) && (queue->head_index == queue->tail_index);
}

inline
boundary_bits_t queue_dequeue(queue_t* queue) {
    boundary_bits_t result;
    if(queue->tail_index < QUEUE_SIZE_PER_PAGE) {
        result = queue->tail_page->entries[queue->tail_index];
        queue->tail_index++;
    } else {
        queue_page_t* new_tail_page = queue->tail_page->next_page;
        deallocate_page(queue->page_allocator, queue->tail_page);
        queue->tail_page = new_tail_page;
        result = queue->tail_page->entries[0];
        queue->tail_index = 1;
    }
    return result;
}

// Moves the first page from donor to donee.
//    Assumes that donor has at least two different pages.
//    Assumes that donee is empty.
void queue_move_first_page(queue_t* donor, queue_t* donee) {
    queue_page_t* new_tail_page = donor->tail_page;
    donor->tail_page = new_tail_page->next_page;
    /* new_tail_page->next_page = NULL; */

    // deallocate_page(donor->page_allocator, &donee->tail_page);
    donee->tail_page = new_tail_page;
    donee->head_page = new_tail_page;
    donee->tail_index = donor->tail_index;
    donee->head_index = QUEUE_SIZE_PER_PAGE;
    donor->tail_index = 0;
}

/*
  typedef struct {
  boundary* data;
  int head;
  int tail;
  int length;
  } queue_t;

  void queue_init(queue_t* queue) {
  queue->head = 0;
  queue->tail = 0;
  queue->length = 2;
  queue->data = malloc(sizeof(boundary)*queue->length);
  for(int k = 0; k < queue->length; k++) {
  queue->data[k] = 0;
  }
  }

  void queue_deinit(queue_t* queue) {
  queue->length = 0;
  free(queue->data);
  }

  void queue_enqueue(queue_t* queue, boundary boundary) {
  if(queue->head == (queue->tail - 1 + queue->length) % queue->length) {
  queue->data[queue->head] = boundary;
  boundary* new_data = malloc(sizeof(boundary)*2*queue->length);
  memset(new_data, 0, sizeof(boundary)*2*queue->length);
  for(int i = 0; i < queue->length; i++) {
  new_data[i] = queue->data[(i + queue->tail) % queue->length];
  }

  queue->tail = 0;
  queue->data[queue->length - 1] = boundary;
  queue->head = queue->length;
  queue->length *= 2;
  free(queue->data);
  queue->data = new_data;
  } else {
  queue->data[queue->head] = boundary;
  queue->head = (queue->head + 1) % queue->length;
  }
  }

  boundary queue_dequeue(queue_t* queue) {
  //printf("Dequeue: %i\n", (int)queue->data[queue->tail]);
  boundary result = queue->data[queue->tail];
  queue->tail = (queue->tail + 1) % queue->length;
  return result;
  }

  uint8_t queue_is_empty(queue_t* queue) {
  return queue->tail == queue->head;
  }
*/
