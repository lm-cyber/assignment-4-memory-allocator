#include <stdio.h>
#define __USE_MISC 1
#include "mem_internals.h"
#include "mem.h"


typedef int (*tester)();

static inline struct block_header* get_header_from_contents(void* contents) {
    return (struct block_header*) ((uint8_t*) contents - offsetof(struct block_header, contents));
}

// Обычное успешное выделение памяти.
int test1() {
    void* heap = heap_init(1000);
    void* block = _malloc(512);

    if (block == NULL || heap == NULL) {
        return 0;
    }

    debug_heap(stdout, heap);
    // Cleanup
    _free(block);
    munmap(heap, size_from_capacity(get_header_from_contents(block)->capacity).bytes);
    return 1;
}

//Освобождение одного блока из нескольких выделенных.
int test2() {
    printf("Init heap\n");
    void* heap = heap_init(1000);
    printf("Done\n");
    void* block1 = _malloc(100);
    printf("Block1done\n");
    struct block_header* h1 = get_header_from_contents(block1);
    void* block2 = _malloc(100);
    printf("Block2done\n");
    void* block3 = _malloc(100);
    printf("Block3done\n");
    struct block_header* h3 = get_header_from_contents(block3);

    if (heap == NULL || block1 == NULL || block2 == NULL || block3 == NULL) {
        return 0;
    }

    debug_heap(stdout, heap);

    size_t next_free_block_size = size_from_capacity(h3->next->capacity).bytes;

    _free(block3);

    if (!h3->is_free || h3->capacity.bytes != next_free_block_size + 100) {
        return 0;
    }

    debug_heap(stdout, heap);
    // Cleanup
    _free(block2);
    _free(block1);
    munmap(heap, size_from_capacity(h1->capacity).bytes);
    return 1;
}

// Освобождение двух блоков из нескольких выделенных.
int test3() {
    void* heap = heap_init(1000);
    void* block1 = _malloc(100);
    struct block_header* h1 = get_header_from_contents(block1);
    void* block2 = _malloc(100);
    struct block_header* h2 = get_header_from_contents(block2);
    void* block3 = _malloc(100);
    struct block_header* h3 = get_header_from_contents(block3);
    void* block4 = _malloc(100);
    struct block_header* h4 = get_header_from_contents(block4);

    if (heap == NULL || block1 == NULL || block2 == NULL || block3 == NULL || block4 == NULL) {
        return 0;
    }

    debug_heap(stdout, heap);

    size_t block3_size = size_from_capacity(h3->capacity).bytes;
    
    _free(block3);
    _free(block2);

    if (!h2->is_free || h2->next != h4 || h2->capacity.bytes != 100 + block3_size) {
        return 0;
    }

    debug_heap(stdout, heap);
    // Cleanup
    _free(block4);
    _free(block1);
    munmap(heap, size_from_capacity(h1->capacity).bytes);
    return 1;
}

// Память закончилась, новый регион памяти расширяет старый.
int test4() {
    size_t full_size = capacity_from_size((block_size) {REGION_MIN_SIZE}).bytes;

    void* heap = heap_init(full_size);

    void* block1 = _malloc(full_size);
    struct block_header* h1 = get_header_from_contents(block1);

    debug_heap(stdout, heap);

    if (heap == NULL || block1 == NULL || h1->capacity.bytes != full_size || h1->next != NULL) {
        printf("Failed to allocate first block\n");
        return 0;
    }

    void* block2 = _malloc(100);
    struct block_header* h2 = get_header_from_contents(block2);

    if (h2 != (struct block_header*) (h1->contents + full_size) || h1->next != h2) {
        return 0;
    }

    debug_heap(stdout, heap);
    // Cleanup
    _free(block2);
    _free(block1);
    munmap(heap, size_from_capacity(h1->capacity).bytes);
    return 1;
}

// Память закончилась, старый регион памяти не расширить из-за другого выделенного 
// диапазона адресов, новый регион выделяется в другом месте
int test5() {
    size_t full_size = capacity_from_size((block_size) {REGION_MIN_SIZE}).bytes;

    void* heap = heap_init(full_size);
    void* block1 = _malloc(full_size);
    struct block_header* h1 = get_header_from_contents(block1);

    if (heap == NULL || block1 == NULL || h1->capacity.bytes != full_size || h1->next != NULL) {
        return 0;
    }

    debug_heap(stdout, heap);

    // Take the next region
    printf("Taking region at address %p\n", (void*) (h1->contents + h1->capacity.bytes));
    void* p = mmap(h1->contents + h1->capacity.bytes, REGION_MIN_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

    if (p == MAP_FAILED) {
        printf("Failed to allocate next region\n");
        return 0;
    }

    void* block2 = _malloc(full_size);

    struct block_header* h2 = get_header_from_contents(block2);

    debug_heap(stdout, heap);

    if (h2 == (void*)(h1->contents + h1->capacity.bytes)) {
        return 0;
    }

    // Free allocated space
    _free(block1);
    munmap(p, h1->contents + h1->capacity.bytes);
    _free(block2);

    return 1;
}

int main() {
    #define TESTS 5
    tester testers[TESTS];
    testers[0] = test1;
    testers[1] = test2;
    testers[2] = test3;
    testers[3] = test4;
    testers[4] = test5;

    char* running = "Running test %d\n";
    char* success = "Test %d passed\n";
    char* fail    = "Test %d failed\n";
    bool res;
 
    for (int i = 0; i<TESTS; i++) {
        printf(running, i+1);
        res = testers[i]();
        printf(res ? success : fail, i+1);
        if (!res) break;
    }
}