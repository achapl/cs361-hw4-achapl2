#include <unistd.h>

void mem_init(void);               
void *mem_sbrk(int incr);

void mem_deinit(void);
void mem_reset_brk(void); 
void *mem_heap_lo(void);
void *mem_heap_hi(void);
size_t mem_heapsize(void);
size_t mem_pagesize(void);

/* hoisted from csapp.h */
void unix_error(char *msg);

/* Dynamic storage allocation wrappers */
void *Malloc(size_t size);
void *Realloc(void *ptr, size_t size);
void *Calloc(size_t nmemb, size_t size);
void Free(void *ptr);
