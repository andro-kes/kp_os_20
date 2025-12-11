#include "../include/allocator.h"
#include "../include/segregated_freelist.h"
#include "../include/mckusick_karels.h"
#include <stdlib.h>
#include <string.h>

struct allocator {
    allocator_type_t type;
    void* impl;
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
    
    void* new_ptr = allocator_alloc(alloc, new_size);
    if (new_ptr) {
        allocator_free(alloc, ptr);
    }
    
    return new_ptr;
}

void allocator_get_stats(allocator_t* alloc, allocator_stats_t* stats) {
    if (!alloc || !stats) return;
    
    memset(stats, 0, sizeof(allocator_stats_t));
}

void allocator_reset_stats(allocator_t* alloc) {
    if (!alloc) return;
}
