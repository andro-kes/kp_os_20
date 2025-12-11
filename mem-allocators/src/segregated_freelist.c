#include "../include/segregated_freelist.h"
#include "../include/allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const size_t SIZE_CLASSES[NUM_SIZE_CLASSES] = {
    16, 32, 64, 128, 256, 512, 1024, 2048
};

typedef struct free_block {
    struct free_block* next;
    size_t size;
} free_block_t;

typedef struct {
    size_t size;
    size_t magic;
} block_header_t;

#define BLOCK_MAGIC 0xDEADBEEF
#define ALIGN_SIZE 8
#define HEADER_SIZE sizeof(block_header_t)

typedef struct {
    allocator_type_t type;
    void* heap; // заранее резервируем участок памяти
    size_t heap_size;
    free_block_t* free_lists[NUM_SIZE_CLASSES]; // свободные блоки для каждого класса
    free_block_t* large_blocks; // доп блоки
    allocator_stats_t stats;
} segregated_freelist_allocator_t;

static int get_size_class(size_t size) {
    for (int i = 0; i < NUM_SIZE_CLASSES; i++) {
        if (size <= SIZE_CLASSES[i]) {
            return i;
        }
    }
    return -1;
}

static size_t align_size(size_t size) {
    return (size + ALIGN_SIZE - 1) & ~(ALIGN_SIZE - 1);
}

allocator_t* segregated_freelist_create(size_t heap_size) {
    segregated_freelist_allocator_t* alloc = malloc(sizeof(segregated_freelist_allocator_t));
    if (!alloc) {
        return NULL;
    }
    
    alloc->type = ALLOCATOR_SEGREGATED_FREELIST;
    alloc->heap_size = heap_size;
    alloc->heap = malloc(heap_size);
    if (!alloc->heap) {
        free(alloc);
        return NULL;
    }
    
    for (int i = 0; i < NUM_SIZE_CLASSES; i++) {
        alloc->free_lists[i] = NULL;
    }
    alloc->large_blocks = NULL;
    
    alloc->large_blocks = (free_block_t*)alloc->heap;
    alloc->large_blocks->next = NULL;
    alloc->large_blocks->size = heap_size;
    
    memset(&alloc->stats, 0, sizeof(allocator_stats_t));
    
    return (allocator_t*)alloc;
}

void segregated_freelist_destroy(allocator_t* alloc) {
    if (!alloc) return;
    
    segregated_freelist_allocator_t* sf_alloc = (segregated_freelist_allocator_t*)alloc;
    free(sf_alloc->heap);
    free(sf_alloc);
}

void* segregated_freelist_alloc(allocator_t* alloc, size_t size) {
    if (!alloc || size == 0) {
        return NULL;
    }
    
    segregated_freelist_allocator_t* sf_alloc = (segregated_freelist_allocator_t*)alloc;
    size_t total_size = align_size(size + HEADER_SIZE);
    int class_idx = get_size_class(total_size);
    
    free_block_t* block = NULL;
    
    if (class_idx >= 0) {
        if (sf_alloc->free_lists[class_idx]) {
            block = sf_alloc->free_lists[class_idx];
            sf_alloc->free_lists[class_idx] = block->next;
        } else {
            free_block_t** prev_ptr = &sf_alloc->large_blocks;
            free_block_t* curr = sf_alloc->large_blocks;
            
            while (curr) {
                if (curr->size >= SIZE_CLASSES[class_idx]) {
                    *prev_ptr = curr->next;
                    
                    size_t block_size = SIZE_CLASSES[class_idx];
                    size_t remaining = curr->size - block_size;
                    
                    block = curr;
                    
                    if (remaining >= SIZE_CLASSES[0]) {
                        free_block_t* remainder = (free_block_t*)((char*)curr + block_size);
                        remainder->size = remaining;
                        remainder->next = sf_alloc->large_blocks;
                        sf_alloc->large_blocks = remainder;
                    }
                    
                    break;
                }
                prev_ptr = &curr->next;
                curr = curr->next;
            }
        }
        
        if (block) {
            block_header_t* header = (block_header_t*)block;
            header->size = total_size;
            header->magic = BLOCK_MAGIC;
            
            sf_alloc->stats.total_allocations++;
            sf_alloc->stats.current_allocated += total_size;
            if (sf_alloc->stats.current_allocated > sf_alloc->stats.peak_allocated) {
                sf_alloc->stats.peak_allocated = sf_alloc->stats.current_allocated;
            }
            
            return (char*)block + HEADER_SIZE;
        }
    } else {
        free_block_t** prev_ptr = &sf_alloc->large_blocks;
        free_block_t* curr = sf_alloc->large_blocks;
        
        while (curr) {
            if (curr->size >= total_size) {
                *prev_ptr = curr->next;
                
                size_t remaining = curr->size - total_size;
                block = curr;
                
                if (remaining >= SIZE_CLASSES[0]) {
                    free_block_t* remainder = (free_block_t*)((char*)curr + total_size);
                    remainder->size = remaining;
                    remainder->next = sf_alloc->large_blocks;
                    sf_alloc->large_blocks = remainder;
                }
                
                block_header_t* header = (block_header_t*)block;
                header->size = total_size;
                header->magic = BLOCK_MAGIC;
                
                sf_alloc->stats.total_allocations++;
                sf_alloc->stats.current_allocated += total_size;
                if (sf_alloc->stats.current_allocated > sf_alloc->stats.peak_allocated) {
                    sf_alloc->stats.peak_allocated = sf_alloc->stats.current_allocated;
                }
                
                return (char*)block + HEADER_SIZE;
            }
            prev_ptr = &curr->next;
            curr = curr->next;
        }
    }
    
    sf_alloc->stats.failed_allocations++;
    return NULL;
}

void segregated_freelist_free(allocator_t* alloc, void* ptr) {
    if (!alloc || !ptr) {
        return;
    }
    
    segregated_freelist_allocator_t* sf_alloc = (segregated_freelist_allocator_t*)alloc;
    block_header_t* header = (block_header_t*)((char*)ptr - HEADER_SIZE);
    
    if (header->magic != BLOCK_MAGIC) {
        fprintf(stderr, "Error: Invalid pointer or corrupted block\n");
        return;
    }
    
    size_t total_size = header->size;
    sf_alloc->stats.total_frees++;
    sf_alloc->stats.current_allocated -= total_size;
    
    int class_idx = get_size_class(total_size);
    free_block_t* block = (free_block_t*)header;
    block->size = total_size;
    
    if (class_idx >= 0 && total_size == SIZE_CLASSES[class_idx]) {
        block->next = sf_alloc->free_lists[class_idx];
        sf_alloc->free_lists[class_idx] = block;
    } else {
        block->next = sf_alloc->large_blocks;
        sf_alloc->large_blocks = block;
    }
}
