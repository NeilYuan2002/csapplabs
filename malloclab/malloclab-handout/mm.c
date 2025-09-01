/*
 * mm.c - this version is based on explicit free list which maintained by LIFO order
 * 
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "MuggleTeam",
    /* First member's full name */
    "Muggle",
    /* First member's email address */
    "571319475@qq.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ 
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  16  /* Extend heap by this amount (bytes) */  
#define MINBLOCKSIZE 24 /* Minimum size for each block */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc)) 

/* Read and write a word at address p */
#define GET(p)       (*(size_t *)(p))          
#define PUT(p, val)  (*(size_t *)(p) = (val))    

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)                   
#define GET_ALLOC(p) (GET(p) & 0x1)                    

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) 
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) 

/* Given block ptr bp, compute address of next and previous free blocks */
#define NEXT_FREEP(bp) (*(void **)((char *)(bp) + WSIZE))
#define PREV_FREEP(bp) (*(void **)((char *)(bp)))
/* $end mallocmacros */

/* Global variables */
static char *heap_listp = 0;
static char *free_listp = 0;

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void printblock(void *bp);
static void checkheap(int verbose);
static void checkblock(void *bp);
static void remove_free_block(void *bp);
static void insert_free_block(void *bp);


/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))



/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{    
  if ((heap_listp = mem_sbrk(2 * MINBLOCKSIZE)) == (void *)-1)
    return -1;

  PUT(heap_listp, 0);
  PUT(heap_listp + WSIZE, PACK(MINBLOCKSIZE, 1));
  PUT(heap_listp + DSIZE, 0);
  PUT(heap_listp + DSIZE + WSIZE, 0);
  PUT(heap_listp + MINBLOCKSIZE, PACK(MINBLOCKSIZE,1));
  PUT(heap_listp + WSIZE + MINBLOCKSIZE, PACK(0,1));  
  free_listp = heap_listp + DSIZE;
  
  //free block of CHUNKSIZE bytes
  if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
    return -1;
  
  return 0;    
}


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     
 */
void *mm_malloc(size_t size)
{
  size_t asize;
  size_t extendsize;
  char *bp;

  if (size <= 0)
    return NULL;

  asize = MAX(ALIGN(size) + DSIZE, MINBLOCKSIZE);

  /* Search for free list */
  if ((bp = find_fit(asize)) != NULL) {
    place(bp, asize);
    return bp;
  }

  /* Not found */
  extendsize = MAX(asize,CHUNKSIZE);
  if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
    return NULL;
  
  place(bp, asize);  
  return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
/* $begin mmfree */
void mm_free(void *bp)
{
  if (!bp)
    return;
  
  size_t size = GET_SIZE(HDRP(bp));

  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  coalesce(bp);

  
}
 /* $end mmfree */
 
 /*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 * delete or insert blocks during coalesce operation to maintain explicit free list
 */


static void *coalesce(void *bp)
{
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))) || PREV_BLKP(bp) == bp;
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {            /* Case 1 */
      insert_free_block(bp);
	return bp;
    }

    else if (prev_alloc && !next_alloc) {      /* Case 2 */     
      size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
      remove_free_block(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */     
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));	
	remove_free_block(PREV_BLKP(bp));
	bp = PREV_BLKP(bp);        
        PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
    }

    else {                                     /* Case 4 */     	
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                GET_SIZE(HDRP(NEXT_BLKP(bp)));
	remove_free_block(PREV_BLKP(bp));
	remove_free_block(NEXT_BLKP(bp));
	bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));	
    }
    	insert_free_block(bp);
	return bp;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = GET_SIZE(HDRP(oldptr)) - 2 * WSIZE;
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}



/*
 * extend_heap - Extend heap with free block and return its block pointer
 */
/* $begin mmextendheap */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; 

    if (size < MINBLOCKSIZE) {
      size = MINBLOCKSIZE;
    }
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;                                        

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */   
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */   
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ 

    /* Coalesce if the previous block was free */
    return coalesce(bp);                                          
}
/* $end mmextendheap */


/*
 * place - Place block of asize bytes at start of free block bp
 *         and split if remainder would be at least minimum block size
 */
/* $begin mmplace */
/* $begin mmplace-proto */
static void place(void *bp, size_t asize)
/* $end mmplace-proto */
{
    size_t csize = GET_SIZE(HDRP(bp));   

    if ((csize - asize) >= MINBLOCKSIZE) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
	remove_free_block(bp);
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
	coalesce(bp);
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
	remove_free_block(bp);
    }
}
/* $end mmplace */ 


/*
 * find_fit - Find a fit for a block with asize bytes
 * simple first-fit implementation
 */
/* $begin mmfirstfit */
/* $begin mmfirstfit-proto */
static void *find_fit(size_t size) {
  void *ptr;

  /* Search from the freelist head to the end */
  for (ptr = free_listp ;GET_ALLOC(HDRP(ptr)) == 0; ptr = NEXT_FREEP(ptr)) {
    if (size <= GET_SIZE(HDRP(ptr))) {
      return ptr;
    }      
  }
  return NULL;
}
/* $end mmfirstfit */


static void printblock(void *bp)
{
    size_t hsize, halloc, fsize, falloc;

    checkheap(0);
    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));

    if (hsize == 0) {
        printf("%p: EOL\n", bp);
        return;
    }

    printf("%p: header: [%ld:%c] footer: [%ld:%c]\n", bp,
           hsize, (halloc ? 'a' : 'f'),
           fsize, (falloc ? 'a' : 'f'));
}

static void checkblock(void *bp)
{
    if ((size_t)bp % 8)
        printf("Error: %p is not doubleword aligned\n", bp);
    if (GET(HDRP(bp)) != GET(FTRP(bp)))
        printf("Error: header does not match footer\n");
}

/*
 * checkheap - Minimal check of the heap for consistency
 */
void checkheap(int verbose)
{
    char *bp = heap_listp;

    if (verbose)
        printf("Heap (%p):\n", heap_listp);

    if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp)))
        printf("Bad prologue header\n");
    checkblock(heap_listp);

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (verbose)
            printblock(bp);
        checkblock(bp);
    }

    if (verbose)
        printblock(bp);
    if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp))))
        printf("Bad epilogue header\n");
}

/*
 *Insert free block into explicit free list by LIFO order
 */
static void insert_free_block(void *bp) {  
  NEXT_FREEP(bp) = free_listp;
  PREV_FREEP(free_listp) = bp;
  PREV_FREEP(bp) = NULL;  
  free_listp = bp;

}
/*
 *Delete block from explicit free list by LIFO order
 */
static void remove_free_block(void *bp) {
  if(PREV_FREEP(bp)){                                                                     
    NEXT_FREEP(PREV_FREEP(bp)) = NEXT_FREEP(bp);                                        
  }  else{                                                                                   
    free_listp = NEXT_FREEP(bp);                                                        
  }
  PREV_FREEP(NEXT_FREEP(bp)) = PREV_FREEP(bp);     
}
