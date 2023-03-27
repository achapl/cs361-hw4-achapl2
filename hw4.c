#include "./hw4-library/memlib.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "./hw4-library/mm.h"
#define WSIZE 4 /* Word and header/footer size (bytes) */            //line:vm:mm:beginconst
#define DSIZE 8                                                      /* Doubleword size (bytes) */


struct memory_region{
  size_t * start;
  size_t * end;
};

struct memory_region global_mem;
struct memory_region stack_mem;

void walk_region_and_mark(void* start, void* end);
void* nearestBlockHead(void* ptr);

// PROVIDED BY US (DO NOT CHANGE)
// ------------------------------
// grabbing the address and size of the global memory region from proc 
void init_global_range(){
  int next = 0;
  char file[100];
  char * line=NULL;
  size_t n=0;
  size_t read_bytes=0;
  size_t start, end;


  sprintf(file, "/proc/%d/maps", getpid());
  FILE * mapfile  = fopen(file, "r");
  if (mapfile==NULL){
    perror("opening maps file failed\n");
    exit(-1);
  }

  int counter=0;
  while ((read_bytes = getline(&line, &n, mapfile)) != -1) {
    // .data of hw4 executable
    if (strstr(line, "hw4") != NULL && strstr(line,"rw-p")){
      sscanf(line, "%lx-%lx", &start, &end);
      global_mem.start = (void*) start;
      global_mem.end = (void*) end;
      break;
    }

  }
  fclose(mapfile);
}

// marking related operations

int is_marked(unsigned int * chunk) {
  return ((*chunk) & 0x2) > 0;
}

void mark(unsigned int * chunk) {
  (*chunk)|=0x2;
}

void clear_mark(unsigned int * chunk) {
  (*chunk)&=(~0x2);
}

// chunk related operations

#define chunk_size(c)  ((*((unsigned int *)c))& ~(unsigned int)7 ) 
void* next_chunk(void* c) { 
  if(chunk_size(c) == 0) {
    fprintf(stderr,"Panic, chunk is of zero size.\n");
  }
  if((c+chunk_size(c)) < mem_heap_hi()) {
    return ((void*)c+chunk_size(c));
  }
  else 
    return 0;
}

int in_use(void *c) { 
  return *(unsigned int *)(c) & 0x1;
}

// FOR YOU TO IMPLEMENT
// --------------------
// the actual collection code
void sweep() {
	#define GET(p) (*(unsigned int *)(p))
	#define WSIZE 4
	#define HDRP(bp) ((char *)(bp)-WSIZE)

	void * chunkPrev = NULL;
	void * chunk = mem_heap_lo();
	void * chunkEnd = mem_heap_hi();
	
	while(chunk < chunkEnd - 3) {
		if (is_marked(chunk) == 0) {
			mm_free(chunk + WSIZE);
		}
		clear_mark(chunk);


		chunkPrev = chunk;
			chunk = next_chunk(chunk);
		if (chunkPrev == chunk){
			fprintf(stderr, "chunk: %p\n", chunk);
			break;
		}
	}
}

// determine if what "looks" like a pointer actually points to an 
// in use block in the heap. if so, return a pointer to its header 
void * is_pointer(void * ptr) {	
	// Here's a helpful print statement that shows some available memory operations and working with pointers
	// fprintf(stderr, "checking for pointeryness of %p between %p and %p\n",ptr, mem_heap_lo(), mem_heap_hi());
	// Check if ptr location is inside of heap memory
	if ((mem_heap_lo() <= ptr) && (ptr < mem_heap_hi())) {
		return ptr;
	}
	return (void *) -1;

  // TODO

}

// walk through memory specified by function arguments and mark
// each chunk
void walk_region_and_mark(void* start, void* end) {
	void* blockPtr = start;// + 4;
	// Iterate through memory blocks
	while (blockPtr < end - 7) {
		if (is_pointer(*(void**)blockPtr) != (void*) -1) {
			void* tempBlockPtr = blockPtr;
			while(is_pointer(*(void**)tempBlockPtr) != (void*) -1) {
				void * blockHead = nearestBlockHead(*(void**)tempBlockPtr);
				if (blockHead != (void * ) -1) {
					mark(blockHead);
				} else {
					fprintf("blockPtr: %p", (void*)blockPtr);
				}
				tempBlockPtr = *(void**)tempBlockPtr;
			}
		}
		blockPtr = (char*) blockPtr + 1;
	}
	
}

void* nearestBlockHead(void * ptr) {
	if ((void*) mem_heap_lo() > (void*) ptr || (void*) ptr >= (void*) mem_heap_hi()){
		return(void * ) -1;
	}
	void* blockPtr = mem_heap_lo(); // Mem_heap_lo() returns address of header of first block allocated
	void* prevPtr  = NULL;
	while((void*) blockPtr < (void*) ptr) {
		prevPtr = blockPtr;
		blockPtr = next_chunk(blockPtr);
	}
	return prevPtr;
}

// PROVIDED BY US (DO NOT CHANGE)
// ------------------------------
// standard initialization 

void init_gc() {
  size_t stack_var;
  init_global_range();
  // since the heap grows down, the end is found first
  stack_mem.end=(size_t *)&stack_var;
}

void gc() {
  size_t stack_var;
  // grows down, so start is a lower address
  stack_mem.start=(size_t *)&stack_var;
  // walk memory regions
  walk_region_and_mark(global_mem.start,global_mem.end);
  walk_region_and_mark(stack_mem.start,stack_mem.end);
  sweep();
}
