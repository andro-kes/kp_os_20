#ifndef SEGREGATED_FREELIST_H
#define SEGREGATED_FREELIST_H

#include "allocator.h"

/* Size classes for segregated free lists (in bytes) */
#define NUM_SIZE_CLASSES 8
extern const size_t SIZE_CLASSES[NUM_SIZE_CLASSES];

/* Segregated free-list specific allocator */
allocator_t* segregated_freelist_create(size_t heap_size);
void segregated_freelist_destroy(allocator_t* alloc);
void* segregated_freelist_alloc(allocator_t* alloc, size_t size);
void segregated_freelist_free(allocator_t* alloc, void* ptr);

#endif /* SEGREGATED_FREELIST_H */
