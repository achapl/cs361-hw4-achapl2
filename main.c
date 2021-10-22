#include "hw4.h"
#include "./hw4-library/mm.h"
#include "./hw4-library/memlib.h"
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_ALLOCATIONS 4000
void *allocs[MAX_ALLOCATIONS] = { (void*) 0x1};

#define chunk_size(c) ((*((unsigned int *)c)) & ~(unsigned long)7)

int timediff()
{
    static struct timeval before;
    struct timeval now;
    gettimeofday(&now, 0);
    int diff = (now.tv_sec - before.tv_sec) * 1000 + (now.tv_usec - before.tv_usec) / 1000;
    before = now;
    return diff;
}

int free_chunks()
{
    void *current = mem_heap_lo();
    int free_chunks = 0;
    while (chunk_size(current))
    {
        if ((*(unsigned int *)current & 1) == 0)
        {
            free_chunks++;
        }
        current += chunk_size(current);
    }
    return free_chunks;
}

int inuse_chunks()
{
    void *current = mem_heap_lo();
    int inuse_chunks = 0;
    while (chunk_size(current))
    {
        if (*(unsigned int *)current & 1)
        {
            inuse_chunks++;
        }
        current += chunk_size(current);
    }
    return inuse_chunks;
}

/* this keeps pointers strictly on the stack, so at i==50, we'll have allocated 100 chunks, and gc'd... 49? */
void *recursive_allocations(int i)
{
    void *ptr = mm_malloc(i);
    if (i == 0)
        return ptr;

    void *ptr2 = recursive_allocations(i - 1);
    if (i == 50)
    {
        printf("Before GC halfway deep in Recursive1: at depth 50, %zu, free %d, inuse %d\n", mem_heapsize(), free_chunks(), inuse_chunks());
        gc();
        printf("After: %zu, free %d, inuse %d\n", mem_heapsize(), free_chunks(), inuse_chunks());
    }
    return ptr;
}

/* here the returned pointer is stored in our local allocation before we return. Hence at depth 50, we're not
     able to GC anything. */
void *recursive_allocations2(int i)
{
    void **ptr = mm_malloc(i * 100 + 128);
    if (i == 0)
        return ptr;

    *ptr = recursive_allocations2(i - 1);
    if (i == 50)
    {
        printf("Before GC halfway deep in Recursive2: At depth 50, %zu, free %d, inuse %d\n", mem_heapsize(), free_chunks(), inuse_chunks());
        gc();
        printf("After: %zu, free %d, inuse %d\n", mem_heapsize(), free_chunks(), inuse_chunks());
    }
    return ptr;
}

int random_up_to(int up_to)
{
    return random() % up_to;
}
int main(int argc, char **argv)
{

    mem_init();
    init_gc();
    timediff();

    printf("Checking global root set handling and general GC functionality\n");
    printf("Heap before first round of allocations: %zu, free %d, inuse %d\n", mem_heapsize(), free_chunks(), inuse_chunks());

    /* the most basic allocation and clearing pointer exercise. This only checks for following the root set pointers one level. */
    for (int i = 0; i < MAX_ALLOCATIONS; i++)
    {
        allocs[i] = mm_malloc(i + 1);
    }
    
    printf("Heap after first round of allocations: %zu, free %d, inuse %d\n", mem_heapsize(), free_chunks(), inuse_chunks());
    gc();
    printf("Heap after gc, before wiping out allocs array: %zu, free %d, inuse %d\n", mem_heapsize(), free_chunks(), inuse_chunks());


    for (int i = 0; i < MAX_ALLOCATIONS; i++)
        allocs[i] = 0;
    gc();
    printf("Heap after wiping out allocs array and then gc(): %zu, free %d, inuse %d\n", mem_heapsize(), free_chunks(), inuse_chunks());
    /* allocations which all point to each other. this checks for proper traversal of the chunk graph. */
    for (int i = 0; i < MAX_ALLOCATIONS; i++)
    {
        allocs[i] = mm_malloc(i + 1);
        if (i > 0)
            *(void **)(allocs[i]) = allocs[i - 1];
    }

    printf("Heap after second round of allocations: %zu, free %d, inuse %d\n", mem_heapsize(), free_chunks(), inuse_chunks());
    for (int i = 0; i < MAX_ALLOCATIONS - 1; i++)
        allocs[i] = 0;
    gc();
    
    // here, since we keep the last entry, which points to the next-to-last and so on, everything should still be around
    printf("Heap after clearing all but one, and gc(): %zu, free %d, inuse %d\n", mem_heapsize(), free_chunks(), inuse_chunks());
    allocs[MAX_ALLOCATIONS - 1] = 0;
    gc();
    printf("Heap after clearing last one, and gc(): %zu, free %d, inuse %d\n", mem_heapsize(), free_chunks(), inuse_chunks());

    /* allocations which all point to each other. this checks for proper traversal of the chunk graph. */
    for (int i = 0; i < MAX_ALLOCATIONS; i++)
    {
        allocs[i] = mm_malloc(i * 2 + 128);
        if (i > 0)
        {
            void *start_of_new_alloc = allocs[i];
            void *start_of_prev_alloc = allocs[i - 1];

            int offset_into_new_alloc = 8 * random_up_to((i * 2 + 120) / 8);
            int offset_into_old_alloc = 8 * random_up_to(((i - 1) * 2 + 120) / 8);
            void **location_of_pointer = (void **)(start_of_new_alloc + offset_into_new_alloc);

            *location_of_pointer = start_of_prev_alloc + offset_into_old_alloc;
        }
    }
    printf("Heap after third round of allocations: %zu, free %d, inuse %d\n", mem_heapsize(), free_chunks(), inuse_chunks());
    for (int i = 0; i < MAX_ALLOCATIONS - 1; i++)
        allocs[i] = 0;
    gc();
    
    // here, since we keep the last entry, which points to the next-to-last and so on, everything should still be around
    printf("Heap after clearing all but one, and gc(): %zu, free %d, inuse %d\n", mem_heapsize(), free_chunks(), inuse_chunks());

    allocs[MAX_ALLOCATIONS - 1] = 0;
    gc();
    printf("Heap after clearing last one, and gc(): %zu, free %d, inuse %d\n", mem_heapsize(), free_chunks(), inuse_chunks());

    printf("Now checking stack root set handling.\n");

    recursive_allocations(100);
    gc();
    printf("After Recursive1 %zu, free %d, inuse %d\n", mem_heapsize(), free_chunks(), inuse_chunks());

    recursive_allocations2(100);
    gc();
    printf("After Recursive2 %zu, free %d, inuse %d\n", mem_heapsize(), free_chunks(), inuse_chunks());
    char buf[100];
}
