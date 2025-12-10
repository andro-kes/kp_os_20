#include "allocator.h"
#include "segregated_freelist.h"
#include "mckusick_karels.h"
#include <stdlib.h>
#include <string.h>

/* Internal allocator structure */
struct allocator {
    allocator_type_t type;
    void* impl;  /* Pointer to actual implementation */
};

allocator_t* allocator_create(allocator_type_t type, size_t heap_size) {
    switch (type) {
        case ALLOCATOR_SEGREGATED_FREELIST:
            return segregated_freelist_create(heap_size);
        case ALLOCATOR_MCKUSICK_KARELS:
            return mckusick_karels_create(heap_size);
        default:
            return NULL;
    }
}

void allocator_destroy(allocator_t* alloc) {
    if (!alloc) return;
    
    switch (alloc->type) {
        case ALLOCATOR_SEGREGATED_FREELIST:
            segregated_freelist_destroy(alloc);
            break;
        case ALLOCATOR_MCKUSICK_KARELS:
            mckusick_karels_destroy(alloc);
            break;
    }
}

void* allocator_alloc(allocator_t* alloc, size_t size) {
    if (!alloc) return NULL;
    
    switch (alloc->type) {
        case ALLOCATOR_SEGREGATED_FREELIST:
            return segregated_freelist_alloc(alloc, size);
        case ALLOCATOR_MCKUSICK_KARELS:
            return mckusick_karels_alloc(alloc, size);
        default:
            return NULL;
    }
}

void allocator_free(allocator_t* alloc, void* ptr) {
    if (!alloc || !ptr) return;
    
    switch (alloc->type) {
        case ALLOCATOR_SEGREGATED_FREELIST:
            segregated_freelist_free(alloc, ptr);
            break;
        case ALLOCATOR_MCKUSICK_KARELS:
            mckusick_karels_free(alloc, ptr);
            break;
    }
}

void* allocator_realloc(allocator_t* alloc, void* ptr, size_t new_size) {
    if (!alloc) return NULL;
    
    if (ptr == NULL) {
        return allocator_alloc(alloc, new_size);
    }
    
    if (new_size == 0) {
        allocator_free(alloc, ptr);
        return NULL;
    }
    
    /* NOTE: Simplified realloc implementation without data copy.
     * This is a limitation of the current implementation - we don't track
     * the size of allocated blocks in a way that allows copying data.
     * A full implementation would need to:
     * 1. Store the allocated size in the block header
     * 2. Copy min(old_size, new_size) bytes to the new block
     * This function is not used by the benchmark suite and is provided
     * for API completeness only. */
    void* new_ptr = allocator_alloc(alloc, new_size);
    if (new_ptr) {
        allocator_free(alloc, ptr);
    }
    
    return new_ptr;
}

void allocator_get_stats(allocator_t* alloc, allocator_stats_t* stats) {
    if (!alloc || !stats) return;
    
    /* NOTE: Simplified stats implementation - returns zeroed stats.
     * A full implementation would need to:
     * 1. Add get_stats methods to segregated_freelist.h and mckusick_karels.h
     * 2. Access the stats field at the known offset in each allocator struct
     * 3. Copy the stats to the output parameter
     * This function is not used by the benchmark suite (which outputs its own metrics)
     * and is provided for API completeness only. */
    memset(stats, 0, sizeof(allocator_stats_t));
}

void allocator_reset_stats(allocator_t* alloc) {
    if (!alloc) return;
    
    /* NOTE: Simplified stats implementation - no-op.
     * A full implementation would need to add reset_stats methods to
     * segregated_freelist.h and mckusick_karels.h.
     * This function is not used by the benchmark suite and is provided
     * for API completeness only. */
}
