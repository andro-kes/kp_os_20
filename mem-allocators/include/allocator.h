#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    ALLOCATOR_SEGREGATED_FREELIST,
    ALLOCATOR_MCKUSICK_KARELS
} allocator_type_t;

typedef struct allocator allocator_t;

allocator_t* allocator_create(allocator_type_t type, size_t heap_size);

void allocator_destroy(allocator_t* alloc);

void* allocator_alloc(allocator_t* alloc, size_t size);

void allocator_free(allocator_t* alloc, void* ptr);

void* allocator_realloc(allocator_t* alloc, void* ptr, size_t new_size);

typedef struct {
    size_t total_allocations;
    size_t total_frees;
    size_t current_allocated;
    size_t peak_allocated;
    size_t failed_allocations;
} allocator_stats_t;

void allocator_get_stats(allocator_t* alloc, allocator_stats_t* stats);

void allocator_reset_stats(allocator_t* alloc);

#endif /* ALLOCATOR_H */
