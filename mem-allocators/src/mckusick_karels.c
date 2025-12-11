#include "../include/mckusick_karels.h"
#include "../include/allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define NUM_BUCKETS 8

// page
typedef struct page {
    struct page* next;
    size_t bucket_size; // size of objects in this page
    unsigned char* free_bitmap; // bitmap of free objects
    size_t num_objects; // number of objects per page
    size_t free_count; // number of free objects
    void* data; // pointer to page data
} page_t;

// header of block
typedef struct {
    page_t* page; // what page  
    size_t object_index; // object index
    size_t magic; // != MK_BLOCK_MAGIC
} mk_block_header_t;

#define MK_BLOCK_MAGIC 0xBEEFCAFE // sub for control
#define MK_ALIGN_SIZE 8 // 8 byte
#define MK_HEADER_SIZE sizeof(mk_block_header_t) // for calculate address


typedef struct {
    allocator_type_t type;
    void* heap; // указатель на сырой кусок памяти
    size_t heap_size; // размер этого куска
    page_t* buckets[NUM_BUCKETS];  
    page_t* full_pages;         
    size_t bucket_sizes[NUM_BUCKETS]; // могут быть разные
    allocator_stats_t stats;
} mckusick_karels_allocator_t;

static void init_bucket_sizes(size_t* bucket_sizes) {
    bucket_sizes[0] = 16;
    bucket_sizes[1] = 32;
    bucket_sizes[2] = 64;
    bucket_sizes[3] = 128;
    bucket_sizes[4] = 256;
    bucket_sizes[5] = 512;
    bucket_sizes[6] = 1024;
    bucket_sizes[7] = 2048;
}

static int get_bucket_index(size_t size, const size_t* bucket_sizes) {
    for (int i = 0; i < NUM_BUCKETS; i++) {
        if (size <= bucket_sizes[i]) {
            return i;
        }
    }
    return -1;  
}

static size_t mk_align_size(size_t size) {
    // округляем до 8 кратного числа
    // ~ обнуляет первые три бита
    return (size + MK_ALIGN_SIZE - 1) & ~(MK_ALIGN_SIZE - 1);
}

static page_t* create_page(size_t bucket_size, void* heap_start, void* heap_end) {
    size_t page_desc_size = sizeof(page_t);
    size_t object_size = bucket_size + MK_HEADER_SIZE;
    size_t num_objects = (PAGE_SIZE - page_desc_size) / object_size;
    
    if (num_objects == 0) {
        num_objects = 1;
    }
    
    size_t bitmap_size = (num_objects + 7) / 8;
    size_t total_size = page_desc_size + bitmap_size + num_objects * object_size;
    
    page_t* page = (page_t*)malloc(sizeof(page_t));
    if (!page) return NULL;
    
    page->data = malloc(num_objects * object_size);
    if (!page->data) {
        free(page);
        return NULL;
    }
    
    page->free_bitmap = (unsigned char*)malloc(bitmap_size);
    if (!page->free_bitmap) {
        free(page->data);
        free(page);
        return NULL;
    }
    
    page->bucket_size = bucket_size;
    page->num_objects = num_objects;
    page->free_count = num_objects;
    page->next = NULL;
    
    memset(page->free_bitmap, 0xFF, bitmap_size);
    
    return page;
}

// ищет первый свободный слот
static int find_free_object(page_t* page) {
    size_t bitmap_size = (page->num_objects + 7) / 8;
    
    for (size_t i = 0; i < page->num_objects; i++) {
        size_t byte_idx = i / 8;
        size_t bit_idx = i % 8;
        
        // 1 - free
        // 0 - not free
        if (page->free_bitmap[byte_idx] & (1 << bit_idx)) {
            return i;
        }
    }
    
    return -1;
}

static void mark_allocated(page_t* page, int obj_idx) {
    size_t byte_idx = obj_idx / 8;
    size_t bit_idx = obj_idx % 8;
    page->free_bitmap[byte_idx] &= ~(1 << bit_idx);
    page->free_count--;
}

static void mark_free(page_t* page, int obj_idx) {
    size_t byte_idx = obj_idx / 8;
    size_t bit_idx = obj_idx % 8;
    page->free_bitmap[byte_idx] |= (1 << bit_idx);
    page->free_count++;
}

allocator_t* mckusick_karels_create(size_t heap_size) {
    mckusick_karels_allocator_t* alloc = malloc(sizeof(mckusick_karels_allocator_t));
    if (!alloc) {
        return NULL;
    }
    
    alloc->type = ALLOCATOR_MCKUSICK_KARELS;
    alloc->heap_size = heap_size;
    alloc->heap = malloc(heap_size);
    if (!alloc->heap) {
        free(alloc);
        return NULL;
    }
    
    init_bucket_sizes(alloc->bucket_sizes);
    
    for (int i = 0; i < NUM_BUCKETS; i++) {
        alloc->buckets[i] = NULL;
    }
    alloc->full_pages = NULL;
    
    memset(&alloc->stats, 0, sizeof(allocator_stats_t));
    
    return (allocator_t*)alloc;
}

void mckusick_karels_destroy(allocator_t* alloc) {
    if (!alloc) return;
    
    mckusick_karels_allocator_t* mk_alloc = (mckusick_karels_allocator_t*)alloc;
    
    for (int i = 0; i < NUM_BUCKETS; i++) {
        page_t* curr = mk_alloc->buckets[i];
        while (curr) {
            page_t* next = curr->next;
            free(curr->free_bitmap);
            free(curr->data);
            free(curr);
            curr = next;
        }
    }
    
    page_t* curr = mk_alloc->full_pages;
    while (curr) {
        page_t* next = curr->next;
        free(curr->free_bitmap);
        free(curr->data);
        free(curr);
        curr = next;
    }
    
    free(mk_alloc->heap);
    free(mk_alloc);
}

// summary
void* mckusick_karels_alloc(allocator_t* alloc, size_t size) {
    if (!alloc || size == 0) {
        return NULL;
    }
    
    mckusick_karels_allocator_t* mk_alloc = (mckusick_karels_allocator_t*)alloc;
    
    int bucket_idx = get_bucket_index(size, mk_alloc->bucket_sizes);
    if (bucket_idx < 0) {
        mk_alloc->stats.failed_allocations++;
        return NULL;
    }
    
    size_t bucket_size = mk_alloc->bucket_sizes[bucket_idx];
    
    page_t* page = mk_alloc->buckets[bucket_idx];
    if (!page || page->free_count == 0) {
        page = create_page(bucket_size, mk_alloc->heap, 
                          (char*)mk_alloc->heap + mk_alloc->heap_size);
        if (!page) {
            mk_alloc->stats.failed_allocations++;
            return NULL;
        }
        
        page->next = mk_alloc->buckets[bucket_idx];
        mk_alloc->buckets[bucket_idx] = page;
    }
    
    int obj_idx = find_free_object(page);
    if (obj_idx < 0) {
        mk_alloc->stats.failed_allocations++;
        return NULL;
    }
    
    mark_allocated(page, obj_idx);
    
    size_t object_size = bucket_size + MK_HEADER_SIZE;
    void* obj_ptr = (char*)page->data + obj_idx * object_size;
    
    mk_block_header_t* header = (mk_block_header_t*)obj_ptr;
    header->page = page;
    header->object_index = obj_idx;
    header->magic = MK_BLOCK_MAGIC;
    
    mk_alloc->stats.total_allocations++;
    mk_alloc->stats.current_allocated += bucket_size;
    if (mk_alloc->stats.current_allocated > mk_alloc->stats.peak_allocated) {
        mk_alloc->stats.peak_allocated = mk_alloc->stats.current_allocated;
    }
    
    if (page->free_count == 0) {
        mk_alloc->buckets[bucket_idx] = page->next;
        page->next = mk_alloc->full_pages;
        mk_alloc->full_pages = page;
    }
    
    return (char*)obj_ptr + MK_HEADER_SIZE;
}

void mckusick_karels_free(allocator_t* alloc, void* ptr) {
    if (!alloc || !ptr) {
        return;
    }
    
    mckusick_karels_allocator_t* mk_alloc = (mckusick_karels_allocator_t*)alloc;
    mk_block_header_t* header = (mk_block_header_t*)((char*)ptr - MK_HEADER_SIZE);
    
    if (header->magic != MK_BLOCK_MAGIC) {
        fprintf(stderr, "Error: Invalid pointer or corrupted block\n");
        return;
    }
    
    page_t* page = header->page;
    int obj_idx = header->object_index;
    
    if (page->free_count == 0) {
        page_t** prev_ptr = &mk_alloc->full_pages;
        page_t* curr = mk_alloc->full_pages;
        while (curr) {
            if (curr == page) {
                *prev_ptr = curr->next;
                break;
            }
            prev_ptr = &curr->next;
            curr = curr->next;
        }
        
        int bucket_idx = get_bucket_index(page->bucket_size, mk_alloc->bucket_sizes);
        page->next = mk_alloc->buckets[bucket_idx];
        mk_alloc->buckets[bucket_idx] = page;
    }
    
    mark_free(page, obj_idx);
    
    mk_alloc->stats.total_frees++;
    mk_alloc->stats.current_allocated -= page->bucket_size;
}
