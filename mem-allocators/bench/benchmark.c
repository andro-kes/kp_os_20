#include "allocator.h"
#include "segregated_freelist.h"
#include "mckusick_karels.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define DEFAULT_HEAP_SIZE (10 * 1024 * 1024)  /* 10 MB */
#define MAX_ALLOCS 10000

/* Benchmark scenarios */
typedef enum {
    BENCH_SEQUENTIAL,
    BENCH_RANDOM,
    BENCH_MIXED,
    BENCH_STRESS
} benchmark_type_t;

/* Get current time in microseconds */
static double get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000.0 + tv.tv_usec;
}

/* Benchmark result */
typedef struct {
    const char* allocator_name;
    const char* benchmark_name;
    double time_us;
    size_t operations;
    double ops_per_sec;
} benchmark_result_t;

/* Print CSV header */
void print_csv_header(void) {
    printf("Allocator,Benchmark,Time_us,Operations,Ops_per_sec\n");
}

/* Print benchmark result as CSV */
void print_result_csv(const benchmark_result_t* result) {
    printf("%s,%s,%.2f,%zu,%.2f\n",
           result->allocator_name,
           result->benchmark_name,
           result->time_us,
           result->operations,
           result->ops_per_sec);
}

/* Benchmark: Sequential allocations and frees */
void benchmark_sequential(allocator_t* alloc, const char* alloc_name, 
                         size_t num_ops, FILE* output) {
    double start = get_time_us();
    
    for (size_t i = 0; i < num_ops / 2; i++) {
        void* ptr = allocator_alloc(alloc, 64);
        if (ptr) {
            allocator_free(alloc, ptr);
        }
    }
    
    double end = get_time_us();
    double elapsed = end - start;
    
    benchmark_result_t result = {
        .allocator_name = alloc_name,
        .benchmark_name = "Sequential",
        .time_us = elapsed,
        .operations = num_ops / 2,
        .ops_per_sec = (num_ops / 2) / (elapsed / 1000000.0)
    };
    
    if (output) {
        fprintf(output, "%s,%s,%.2f,%zu,%.2f\n",
                result.allocator_name, result.benchmark_name,
                result.time_us, result.operations, result.ops_per_sec);
    } else {
        print_result_csv(&result);
    }
}

/* Benchmark: Random size allocations */
void benchmark_random(allocator_t* alloc, const char* alloc_name, 
                     size_t num_ops, FILE* output) {
    void* ptrs[1000];
    int ptr_count = 0;
    
    srand(42);  /* Fixed seed for reproducibility */
    double start = get_time_us();
    
    for (size_t i = 0; i < num_ops; i++) {
        int action = rand() % 2;
        
        if (action == 0 && ptr_count < 1000) {
            /* Allocate */
            size_t size = 16 + (rand() % 1024);
            void* ptr = allocator_alloc(alloc, size);
            if (ptr) {
                ptrs[ptr_count++] = ptr;
            }
        } else if (ptr_count > 0) {
            /* Free */
            int idx = rand() % ptr_count;
            allocator_free(alloc, ptrs[idx]);
            ptrs[idx] = ptrs[--ptr_count];
        }
    }
    
    /* Free remaining */
    for (int i = 0; i < ptr_count; i++) {
        allocator_free(alloc, ptrs[i]);
    }
    
    double end = get_time_us();
    double elapsed = end - start;
    
    benchmark_result_t result = {
        .allocator_name = alloc_name,
        .benchmark_name = "Random",
        .time_us = elapsed,
        .operations = num_ops,
        .ops_per_sec = num_ops / (elapsed / 1000000.0)
    };
    
    if (output) {
        fprintf(output, "%s,%s,%.2f,%zu,%.2f\n",
                result.allocator_name, result.benchmark_name,
                result.time_us, result.operations, result.ops_per_sec);
    } else {
        print_result_csv(&result);
    }
}

/* Benchmark: Mixed allocation patterns */
void benchmark_mixed(allocator_t* alloc, const char* alloc_name, 
                    size_t num_ops, FILE* output) {
    void* ptrs[500];
    
    double start = get_time_us();
    
    /* Phase 1: Allocate many small blocks */
    for (int i = 0; i < 500; i++) {
        ptrs[i] = allocator_alloc(alloc, 32);
    }
    
    /* Phase 2: Free every other block */
    for (int i = 0; i < 500; i += 2) {
        allocator_free(alloc, ptrs[i]);
        ptrs[i] = NULL;
    }
    
    /* Phase 3: Allocate larger blocks in freed spaces */
    for (int i = 0; i < 500; i += 2) {
        ptrs[i] = allocator_alloc(alloc, 128);
    }
    
    /* Phase 4: Free all */
    for (int i = 0; i < 500; i++) {
        if (ptrs[i]) {
            allocator_free(alloc, ptrs[i]);
        }
    }
    
    double end = get_time_us();
    double elapsed = end - start;
    
    benchmark_result_t result = {
        .allocator_name = alloc_name,
        .benchmark_name = "Mixed",
        .time_us = elapsed,
        .operations = 2000,  /* Approximate operation count */
        .ops_per_sec = 2000 / (elapsed / 1000000.0)
    };
    
    if (output) {
        fprintf(output, "%s,%s,%.2f,%zu,%.2f\n",
                result.allocator_name, result.benchmark_name,
                result.time_us, result.operations, result.ops_per_sec);
    } else {
        print_result_csv(&result);
    }
}

/* Benchmark: Stress test with many allocations */
void benchmark_stress(allocator_t* alloc, const char* alloc_name, 
                     size_t num_ops, FILE* output) {
    void* ptrs[MAX_ALLOCS];
    int allocated = 0;
    
    double start = get_time_us();
    
    /* Allocate as many as possible */
    for (int i = 0; i < MAX_ALLOCS && i < num_ops; i++) {
        ptrs[i] = allocator_alloc(alloc, 256);
        if (ptrs[i]) {
            allocated++;
        } else {
            break;
        }
    }
    
    /* Free all */
    for (int i = 0; i < allocated; i++) {
        allocator_free(alloc, ptrs[i]);
    }
    
    double end = get_time_us();
    double elapsed = end - start;
    
    benchmark_result_t result = {
        .allocator_name = alloc_name,
        .benchmark_name = "Stress",
        .time_us = elapsed,
        .operations = allocated * 2,  /* Allocs + frees */
        .ops_per_sec = (allocated * 2) / (elapsed / 1000000.0)
    };
    
    if (output) {
        fprintf(output, "%s,%s,%.2f,%zu,%.2f\n",
                result.allocator_name, result.benchmark_name,
                result.time_us, result.operations, result.ops_per_sec);
    } else {
        print_result_csv(&result);
    }
}

/* Run all benchmarks for a given allocator */
void run_benchmarks(allocator_type_t type, const char* name, 
                   size_t num_ops, FILE* output) {
    printf("Running benchmarks for %s...\n", name);
    
    allocator_t* alloc = allocator_create(type, DEFAULT_HEAP_SIZE);
    if (!alloc) {
        fprintf(stderr, "Failed to create allocator: %s\n", name);
        return;
    }
    
    benchmark_sequential(alloc, name, num_ops, output);
    allocator_destroy(alloc);
    
    alloc = allocator_create(type, DEFAULT_HEAP_SIZE);
    benchmark_random(alloc, name, num_ops, output);
    allocator_destroy(alloc);
    
    alloc = allocator_create(type, DEFAULT_HEAP_SIZE);
    benchmark_mixed(alloc, name, num_ops, output);
    allocator_destroy(alloc);
    
    alloc = allocator_create(type, DEFAULT_HEAP_SIZE);
    benchmark_stress(alloc, name, num_ops, output);
    allocator_destroy(alloc);
}

void print_usage(const char* prog_name) {
    printf("Usage: %s [OPTIONS]\n", prog_name);
    printf("Options:\n");
    printf("  -a, --allocator <type>   Allocator type: segregated, mckusick, all (default: all)\n");
    printf("  -n, --num-ops <number>   Number of operations (default: 10000)\n");
    printf("  -o, --output <file>      Output CSV file (default: stdout)\n");
    printf("  -h, --help               Show this help message\n");
}

int main(int argc, char* argv[]) {
    allocator_type_t alloc_type = -1;
    size_t num_ops = 10000;
    const char* output_file = NULL;
    bool run_all = true;
    
    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--allocator") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Missing allocator type\n");
                print_usage(argv[0]);
                return 1;
            }
            const char* type = argv[++i];
            if (strcmp(type, "segregated") == 0) {
                alloc_type = ALLOCATOR_SEGREGATED_FREELIST;
                run_all = false;
            } else if (strcmp(type, "mckusick") == 0) {
                alloc_type = ALLOCATOR_MCKUSICK_KARELS;
                run_all = false;
            } else if (strcmp(type, "all") == 0) {
                run_all = true;
            } else {
                fprintf(stderr, "Error: Unknown allocator type: %s\n", type);
                print_usage(argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--num-ops") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Missing number of operations\n");
                print_usage(argv[0]);
                return 1;
            }
            num_ops = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Missing output file\n");
                print_usage(argv[0]);
                return 1;
            }
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    FILE* output = NULL;
    if (output_file) {
        output = fopen(output_file, "w");
        if (!output) {
            fprintf(stderr, "Error: Failed to open output file: %s\n", output_file);
            return 1;
        }
    }
    
    printf("=== Memory Allocator Benchmark ===\n");
    printf("Operations per benchmark: %zu\n\n", num_ops);
    
    /* Print CSV header */
    if (output) {
        fprintf(output, "Allocator,Benchmark,Time_us,Operations,Ops_per_sec\n");
    } else {
        print_csv_header();
    }
    
    if (run_all) {
        run_benchmarks(ALLOCATOR_SEGREGATED_FREELIST, 
                      "SegregatedFreeList", num_ops, output);
        run_benchmarks(ALLOCATOR_MCKUSICK_KARELS, 
                      "McKusickKarels", num_ops, output);
    } else {
        const char* name = (alloc_type == ALLOCATOR_SEGREGATED_FREELIST) ? 
                          "SegregatedFreeList" : "McKusickKarels";
        run_benchmarks(alloc_type, name, num_ops, output);
    }
    
    if (output) {
        fclose(output);
        printf("\nResults written to: %s\n", output_file);
    }
    
    printf("\nBenchmark complete!\n");
    return 0;
}
