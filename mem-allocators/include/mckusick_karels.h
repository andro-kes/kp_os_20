#ifndef MCKUSICK_KARELS_H
#define MCKUSICK_KARELS_H

#include "allocator.h"

#define PAGE_SIZE 4096
#define MIN_BUCKET_SIZE 16
#define MAX_BUCKET_SIZE 2048

allocator_t* mckusick_karels_create(size_t heap_size);
void mckusick_karels_destroy(allocator_t* alloc);
void* mckusick_karels_alloc(allocator_t* alloc, size_t size);
void mckusick_karels_free(allocator_t* alloc, void* ptr);

#endif
