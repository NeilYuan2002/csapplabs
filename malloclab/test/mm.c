/**
 * @file mm.c Explicit free list implementation of malloc
 * @author Somsubhra (201101056@daiict.ac.in)
 * @brief The following implemenation of memory management is based on doubly linked
 * explicit free list and first fit algorithm.
 *
 * => In mm_malloc, the block is allocated by traversing the free list and finding the
 *    first free block that has size greater than the requested size. If the free
 *    block is too big then the free block is split and the first part is allocated
 *    and the rest of the part is kept free. The headers and footers of the new blocks
 *    are changed accordingly.
 *
 * => In mm_free, the block to be freed is found and its header/ footer are changed.
 *    The allocated bit is set to 0. Then the freed block is placed at the beginning of
 *    the free list. If the adjacent blocks are also free, then the freed block
 *    is coalesced with them.
 *
 * => In mm-realloc, depending on the newly requested size, if the new size is smaller
 *    than the old size, then the block is shrunk updating the header/footer. If the
 *    new size is greater than old size, then we allocate a new block with malloc, copy
 *    the old data into the new block and free the old block. If the new size is same
 *    as the old size, then the same block is returned.
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
    "Team?",
    /* First member's full name */
    "Somsubhra Bairi",
    /* First member's email address */
    "201101056@daiict.ac.in",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*Additional Macros defined*/
#define WSIZE 4                                                                             //Size of a word
#define DSIZE 8                                                                             //Size of a double word
#define CHUNKSIZE 16                                                                        //Initial heap size
#define OVERHEAD 24                                                                         //The minimum block size
#define MAX(x ,y)  ((x) > (y) ? (x) : (y))                                                  //Finds the maximum of two numbers
#define PACK(size, alloc)  ((size) | (alloc))                                               //Put the size and allocated byte into one word
#define GET(p)  (*(size_t *)(p))                                                            //Read the word at address p
#define PUT(p, value)  (*(size_t *)(p) = (value))                                           //Write the word at address p
#define GET_SIZE(p)  (GET(p) & ~0x7)                                                        //Get the size from header/footer
#define GET_ALLOC(p)  (GET(p) & 0x1)                                                        //Get the allocated bit from header/footer
#define HDRP(bp)  ((void *)(bp) - WSIZE)                                                    //Get the address of the header of a block
#define FTRP(bp)  ((void *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)                               //Get the address of the footer of a block
#define NEXT_BLKP(bp)  ((void *)(bp) + GET_SIZE(HDRP(bp)))                                  //Get the address of the next block
#define PREV_BLKP(bp)  ((void *)(bp) - GET_SIZE(HDRP(bp) - WSIZE))                          //Get the address of the previous block
#define NEXT_FREEP(bp)  (*(void **)(bp + DSIZE))                                            //Get the address of the next free block
#define PREV_FREEP(bp)  (*(void **)(bp))                                                    //Get the address of the previous free block

static char *heap_listp = 0;                                                                //Pointer to the first block
static char *free_listp = 0;                                                                //Pointer to the first free block

//Function prototypes for helper routines
static void *extend_heap(size_t words);
static void place(void *bp, size_t size);
static void *find_fit(size_t size);
static void *coalesce(void *bp);
static void insert_at_front(void *bp);
static void remove_block(void *bp);
static int check_block(void *bp);

/**
 * @brief mm_init Initializes the malloc
 * @return Return 0 if successful -1 if unsucessful
 */
int mm_init(void)
{
    if((heap_listp = mem_sbrk(2 * OVERHEAD)) == NULL){                                      //Return error if unable to get heap space
        return -1;
    }

    PUT(heap_listp, 0);                                                                     //Put the Padding at the start of heap
    PUT(heap_listp + WSIZE, PACK(OVERHEAD, 1));                                             //Put the header block of the prologue
    PUT(heap_listp + DSIZE, 0);                                                             //Put the previous pointer
    PUT(heap_listp + DSIZE + WSIZE, 0);                                                     //Put the next pointer
    PUT(heap_listp + OVERHEAD, PACK(OVERHEAD, 1));                                          //Put the footer block of the prologue
    PUT(heap_listp + WSIZE + OVERHEAD, PACK(0, 1));                                         //Put the header block of the epilogue
    free_listp = heap_listp + DSIZE;                                                        //Initialize the free list pointer

    if(extend_heap(CHUNKSIZE / WSIZE) == NULL){                                             //Return error if unable to extend heap space
        return -1;
    }

    return 0;
}

/**
 * @brief mm_malloc Allocates a block with atleast the specified size of payload
 * @param size The payload size
 * @return The pointer to the start of the allocated block
 */
void *mm_malloc(size_t size)
{
    size_t adjustedsize;                                                                    //The size of the adjusted block
    size_t extendedsize;                                                                    //The amount by which heap is extended if no fit is found
    char *bp;                                                                               //Stores the block pointer

    if(size <= 0){                                                                          //If requested size is less than 0 then ignore
        return NULL;
    }

    adjustedsize = MAX(ALIGN(size) + DSIZE, OVERHEAD);                                      //Adjust block size to include overhead and alignment requirements

    if((bp = find_fit(adjustedsize))){                                                      //Traverse the free list for the first fit
        place(bp, adjustedsize);                                                            //Place the block in the free list
        return bp;
    }

    extendedsize = MAX(adjustedsize, CHUNKSIZE);                                            //If no fit is found get more memory to extend the heap

    if((bp = extend_heap(extendedsize / WSIZE)) == NULL){                                   //If unable to extend heap space
        return NULL;                                                                        //return null
    }

    place(bp, adjustedsize);                                                                //Place the block in the newly extended space
    return bp;
}

/**
 * @brief mm_free Frees a block
 * @param bp The block to be freed
 */
void mm_free(void *bp)
{
    if(!bp){                                                                                //If block pointer is null
        return;                                                                             //return
    }

    size_t size = GET_SIZE(HDRP(bp));                                                       //Get the total block size

    PUT(HDRP(bp), PACK(size, 0));                                                           //Set the header as unallocated
    PUT(FTRP(bp), PACK(size, 0));                                                           //Set the footer as unallocated
    coalesce(bp);                                                                           //Coalesce and add the block to the free list
}

/**
 * @brief mm_realloc Reallocates a block of memory
 * @param bp The block pointer of the block to be reallocated
 * @param size The size of the block to be reallocated
 * @return The block pointer to the reallocated block
 */
void *mm_realloc(void *bp, size_t size)
{
    size_t oldsize;                                                                         //Holds the old size of the block
    void *newbp;                                                                            //Holds the new block pointer
    size_t adjustedsize = MAX(ALIGN(size) + DSIZE, OVERHEAD);                               //Calciulate the adjusted size

    if(size <= 0){                                                                          //If size is less than 0
        mm_free(bp);                                                                        //Free the block
        return 0;
    }

    if(bp == NULL){                                                                         //If old block pointer is null, then it is malloc
        return mm_malloc(size);
    }

    oldsize = GET_SIZE(HDRP(bp));                                                           //Get the size of the old block

    if(oldsize == adjustedsize){                                                            //If the size of the old block and requested size are same then return the old block pointer
        return bp;
    }

    if(adjustedsize <= oldsize){                                                            //If the size needs to be decreased
        size = adjustedsize;                                                                //Shrink the block

        if(oldsize - size <= OVERHEAD){                                                     //If the new block cannot be formed
            return bp;                                                                      //Return the old block pointer
        }
                                                                                            //If a new block can be formed
        PUT(HDRP(bp), PACK(size, 1));                                                       //Update the size in the header of the reallocated block
        PUT(FTRP(bp), PACK(size, 1));                                                       //Update the size in the header of the reallocated block
        PUT(HDRP(NEXT_BLKP(bp)), PACK(oldsize - size, 1));                                  //Update the size in the header of the next block
        mm_free(NEXT_BLKP(bp));                                                             //Free the next block
        return bp;
    }
                                                                                            //If the block has to be expanded during reallocation
    newbp = mm_malloc(size);                                                                //Allocate a new block

    if(!newbp){                                                                             //If realloc fails the original block is left as it is
        return 0;
    }

    if(size < oldsize){
        oldsize = size;
    }

    memcpy(newbp, bp, oldsize);                                                             //Copy the old data to the new block
    mm_free(bp);                                                                            //Free the old block
    return newbp;
}

/**
 * @brief extend_heap Extends the heap with free block
 * @param words The size to extend the heap by
 * @return The block pointer to the frst block in the newly acquired heap space
 */
static void* extend_heap(size_t words){
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;                               //Allocate even number of  words to maintain alignment

    if(size < OVERHEAD){
        size = OVERHEAD;
    }

    if((long)(bp = mem_sbrk(size)) == -1){                                                  //If error in extending heap space return null
        return NULL;
    }

    PUT(HDRP(bp), PACK(size, 0));                                                           //Put the free block header
    PUT(FTRP(bp), PACK(size, 0));                                                           //Put the free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));                                                   //Put the new epilogue header

    return coalesce(bp);                                                                    //Coalesce if the previous block was free and add the block to the free list
}

/**
 * @brief coalesce Combines the newly freed block with the adjacent free blocks if any
 * @param bp The block pointer to the newly freed block
 * @return The pointer to the coalesced block
 */
static void *coalesce(void *bp){
    size_t previous_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))) || PREV_BLKP(bp) == bp;          //Stores whether the previous block is allocated or not
    size_t next__alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));                                    //Stores whether the next block is allocated or not
    size_t size = GET_SIZE(HDRP(bp));                                                       //Stores the size of the block

    if(previous_alloc && !next__alloc){                                                     //Case 1: The block next to the current block is free
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));                                              //Add the size of the next block to the current block to make it a single block
        remove_block(NEXT_BLKP(bp));                                                        //Remove the next block
        PUT(HDRP(bp), PACK(size, 0));                                                       //Update the new block's header
        PUT(FTRP(bp), PACK(size, 0));                                                       //Update the new block's footer
    }

    else if(!previous_alloc && next__alloc){                                                //Case 2: The block previous to the current block is free
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));                                              //Add the size of the previous block to the current bloxk to make it a single block
        bp = PREV_BLKP(bp);                                                                 //Update the block pointer to the previous block
        remove_block(bp);                                                                   //Remove the previous block
        PUT(HDRP(bp), PACK(size, 0));                                                       //Update the new block's header
        PUT(FTRP(bp), PACK(size, 0));                                                       //Update the new block's footer
    }

    else if(!previous_alloc && !next__alloc){                                               //Case 3: The blocks to the either side of the current block are free
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));              //Add the size of previous and next blocks to the current block to make it single
        remove_block(PREV_BLKP(bp));                                                        //Remove the block previous to the current block
        remove_block(NEXT_BLKP(bp));                                                        //Remove the block next to the current block
        bp = PREV_BLKP(bp);                                                                 //Update the block pointer to the previous block
        PUT(HDRP(bp), PACK(size, 0));                                                       //Update the new block's header
        PUT(FTRP(bp), PACK(size, 0));                                                       //Update the new block's footer
    }
    insert_at_front(bp);                                                                    //Insert the block to the start of free list
    return bp;
}

/**
 * @brief insert_at_front Inserts a block at the front of the free list
 * @param bp The pointer of the block to be added at the front of the free list
 */
static void insert_at_front(void *bp){
    NEXT_FREEP(bp) = free_listp;                                                            //Sets the next pointer to the start of the free list
    PREV_FREEP(free_listp) = bp;                                                            //Sets the current's previous to the new block
    PREV_FREEP(bp) = NULL;                                                                  //Set the previosu free pointer to NULL
    free_listp = bp;                                                                        //Sets the start of the free list as the new block
}

/**
 * @brief remove_block Removes a block from the free list
 * @param bp The pointer to the block to be removed from the free list
 */
static void remove_block(void *bp){
    if(PREV_FREEP(bp)){                                                                     //If there is a previous block
        NEXT_FREEP(PREV_FREEP(bp)) = NEXT_FREEP(bp);                                        //Set the next pointer of the previous block to next block
    }

    else{                                                                                   //If there is no previous block
        free_listp = NEXT_FREEP(bp);                                                        //Set the free list to the next block
    }

    PREV_FREEP(NEXT_FREEP(bp)) = PREV_FREEP(bp);                                            //Set the previous block's pointer of the next block to the previous block
}

/**
 * @brief find_fit Finds a fit for the block of a given size
 * @param size The size of the block to be fit
 * @return The pointer to the block used for allocation
 */
static void *find_fit(size_t size){
    void *bp;

    for(bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_FREEP(bp)){                    //Traverse the entire free list
        if(size <= GET_SIZE(HDRP(bp))){                                                     //If size fits in the available free block
            return bp;                                                                      //Return the block pointer
        }
    }

    return NULL;                                                                            //If no fit is found return NULL
}

/**
 * @brief place Place a block of specified size to start of free block
 * @param bp The block pointer to the free block
 * @param size The size of the block to be placed
 */
static void place(void *bp, size_t size){
    size_t totalsize = GET_SIZE(HDRP(bp));                                                  //Get the total size of thefree block

    if((totalsize - size) >= OVERHEAD){                                                     //If the difference between the total size and requested size is more than overhead, split the block
        PUT(HDRP(bp), PACK(size, 1));                                                       //Put the header of the allocated block
        PUT(FTRP(bp), PACK(size, 1));                                                       //Put the footer of the allocated block
        remove_block(bp);                                                                   //Remove the allocated block
        bp = NEXT_BLKP(bp);                                                                 //The block pointer of the free block created by the partition
        PUT(HDRP(bp), PACK(totalsize - size, 0));                                           //Put the header of the new unallocated block
        PUT(FTRP(bp), PACK(totalsize - size, 0));                                           //Put the footer of the new unallocated block
        coalesce(bp);                                                                       //Coalesce the new free block with the adjacent free blocks
    }

    else{                                                                                   //If the remaining space is not enough for a free block then donot split the block
        PUT(HDRP(bp), PACK(totalsize, 1));                                                  //Put the header of the block
        PUT(FTRP(bp), PACK(totalsize, 1));                                                  //Put the footer of the block
        remove_block(bp);                                                                   //Remove the allocated block
    }
}

/**
 * @brief mm_check Checks the heap for inconsistency
 * @return Returns 0 if consistent, -1 is inconsistent
 */
int mm_check(void){
    void *bp = heap_listp;                                                                  //Points to the first block in the heap
    printf("Heap (%p): \n", heap_listp);                                                    //Print the address of the heap

    if((GET_SIZE(HDRP(heap_listp)) != OVERHEAD) || !GET_ALLOC(HDRP(heap_listp))){           //If the first block's header's size or allocated bit is wrong
        printf("Fatal: Bad prologue header\n");                                             //Throw error
        return -1;
    }
    if(check_block(heap_listp) == -1){                                                      //Check the block for consistency
        return -1;
    }

    for(bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_FREEP(bp)){                    //Check all the blocks of free list for consistency
         if(check_block(bp) == -1){                                                         //If inconsistency
                return -1;                                                                  //Throw error
         }
    }

    return 0;                                                                               //No inconsistency
}

/**
 * @brief check_block Checks a block for consistency
 * @param bp The pointer of the block to be checked for consistency
 * @return Returns 0 if consistent, -1 if inconsistent
 */
static int check_block(void *bp){
    if(NEXT_FREEP(bp) < mem_heap_lo() || NEXT_FREEP(bp) > mem_heap_hi()){                   //If next free pointer is out of heap range
        printf("Fatal: Next free pointer %p is out of bounds\n", NEXT_FREEP(bp));           //Throw error
        return -1;
    }

    if(PREV_FREEP(bp) < mem_heap_lo() || PREV_FREEP(bp) > mem_heap_hi()){                   //If previous free pointer is out of heap range
        printf("Fatal: Previous free pointer %p is oiut of bounds", PREV_FREEP(bp));        //Throw error
        return -1;
    }

    if((size_t)bp % 8){                                                                     //If there is no alignment
        printf("Fatal: %p is not aligned", bp);                                             //Throw error
        return -1;
    }

    if(GET(HDRP(bp)) != GET(FTRP(bp))){                                                     //If header and footer mismatch
        printf("Fatal: Header and footer mismatch");                                        //Throw error
        return -1;
    }

    return 0;                                                                               //Block is consistent
}
