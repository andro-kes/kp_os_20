#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>

/* Allocator types */
typedef enum {
    ALLOCATOR_SEGREGATED_FREELIST,
    ALLOCATOR_MCKUSICK_KARELS
} allocator_type_t;

/* Allocator interface */
typedef struct allocator allocator_t;

/* Create allocator of specified type with given heap size */
allocator_t* allocator_create(allocator_type_t type, size_t heap_size);

/* Destroy allocator and free all resources */
void allocator_destroy(allocator_t* alloc);

/* Allocate memory block of given size */
void* allocator_alloc(allocator_t* alloc, size_t size);

/* Free previously allocated memory block */
void allocator_free(allocator_t* alloc, void* ptr);

/* Reallocate memory block to new size */
void* allocator_realloc(allocator_t* alloc, void* ptr, size_t new_size);

/* Get allocator statistics */
typedef struct {
    size_t total_allocations;
    size_t total_frees;
    size_t current_allocated;
    size_t peak_allocated;
    size_t failed_allocations;
} allocator_stats_t;

void allocator_get_stats(allocator_t* alloc, allocator_stats_t* stats);

/* Reset allocator statistics */
void allocator_reset_stats(allocator_t* alloc);

#endif /* ALLOCATOR_H */
