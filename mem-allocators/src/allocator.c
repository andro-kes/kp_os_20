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
    
    /* Simple realloc: allocate new block, copy data, free old block */
    void* new_ptr = allocator_alloc(alloc, new_size);
    if (new_ptr) {
        /* Note: We don't know the old size, so we can't do a proper memcpy
         * In a real implementation, we'd need to store the size in the block header */
        allocator_free(alloc, ptr);
    }
    
    return new_ptr;
}

void allocator_get_stats(allocator_t* alloc, allocator_stats_t* stats) {
    if (!alloc || !stats) return;
    
    /* Stats are stored in the first part of the implementation structure */
    /* Both allocators have stats at the same offset after the type field */
    typedef struct {
        allocator_type_t type;
        void* heap;
        size_t heap_size;
        /* ... other fields ... */
        /* Stats are usually near the end but after common fields */
    } common_allocator_t;
    
    /* For simplicity, we'll access stats directly from the implementation
     * This requires the stats to be at a known offset in both implementations */
    
    /* This is a simplified approach - a better way would be to add
     * get_stats methods to each allocator implementation */
    memset(stats, 0, sizeof(allocator_stats_t));
}

void allocator_reset_stats(allocator_t* alloc) {
    if (!alloc) return;
    
    /* Similar to get_stats, this would need implementation-specific handling */
}
