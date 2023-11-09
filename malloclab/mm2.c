/* 
 * Simple, 32-bit and 64-bit clean allocator based on implicit free
 * lists, first-fit placement, and boundary tag coalescing, as described
 * in the CS:APP3e text. Blocks must be aligned to doubleword (8 byte) 
 * boundaries. Minimum block size is 16 bytes. 
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mm.h"
#include "memlib.h"

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/*
 * If NEXT_FIT defined use next fit search, else use first-fit search 
 */
#define NEXT_FITx

/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ 
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */  

#define MAX(x, y) ((x) > (y)? (x) : (y))  

#define PRALLOC(x)  ((!x) ? 0 : 2)
/* Pack a size and allocated bit into a word */
#define PACK(size, alloc, pralloc)  ((size) | PRALLOC(pralloc) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))            
#define PUT(p, val)  (*(unsigned int *)(p) = (val))    

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)                   
#define GET_ALLOC(p) (GET(p) & 0x1)                    
#define GET_PRALLOC(p)  (GET(p) & 0x2)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) 
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) 

/* pointer to pointer to pred/succ */
#define pPRED(bp)  ((char *)(bp))
#define pSUCC(bp)  ((char *)(bp) + DSIZE)

/* value of pointer to pred/succ */
#define GET_PRED(bp)  (char *)(*(char **)(bp))
#define GET_SUCC(bp)  (char *)(*(char **)(pSUCC(bp)))

/* Set address p to the val of a pointer (8 bytes) */
#define SET_PTR_VAL(p, ptr)  ((*(unsigned long *)(p)) = (unsigned long)(ptr))

/* Read and write the i_th size-class's first pointer */
#define GET_FREE_LIST(i)      (*((free_list) + (i)))
#define SET_FREE_LIST(i, bp)  ((*((free_list) + (i))) = (char *)(bp))

/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */  
static char **free_list;  /* free list */
#define CLASSNUM 31  /* numbers of classes */

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void *place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void addFreeBLK(void *bp, size_t size);
static void delBLK(void *bp);
inline void setNextPralloc(void * bp, size_t pralloc);
/* 
 * mm_init - Initialize the memory manager 
 */
int mm_init(void) 
{
    /* allocate space for and initialize free_list first */
    if ((free_list = mem_sbrk(CLASSNUM * sizeof(char *))) == (void *)-1) 
        return -1;  
    /* in fact 14 classes are enough, a little more for security*/
    for(int i = 0; i < CLASSNUM; i++) SET_FREE_LIST(i, NULL);

    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) 
        return -1;
    PUT(heap_listp, 0);                          /* Alignment padding */
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1, 1)); /* Prologue header */ 
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1, 1)); /* Prologue footer */ 
    PUT(heap_listp + (3*WSIZE), PACK(0, 1, 1));     /* Epilogue header */
    heap_listp += (2*WSIZE);                     

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(1 << 6) == NULL) 
        return -1;
    return 0;
}

/* 
 * malloc - Allocate a block with at least size bytes of payload 
 */
void *malloc(size_t size) 
{
    // printf("malloc called\n");
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;      

    if (heap_listp == 0){
        mm_init();
    }
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= 2 * DSIZE + WSIZE)                                          
        asize = 3 * DSIZE;                                        
    else
        asize = DSIZE * ((size + (DSIZE) + (WSIZE-1)) / DSIZE); 

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {  
        bp = place(bp, asize);                  
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);                 
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)  
        return NULL;                                  
    bp = place(bp, asize);                                 
    return bp;
} 

/* 
 * free - Free a block 
 */
void free(void *bp)
{
   // printf("free called\n");
    if (bp == 0) 
        return;

    size_t size = GET_SIZE(HDRP(bp));
    if (heap_listp == 0){
        mm_init();
    }

    size_t pralloc = GET_PRALLOC(HDRP(bp));

    PUT(HDRP(bp), PACK(size, pralloc, 0));
    PUT(FTRP(bp), PACK(size, pralloc, 0));

    addFreeBLK(bp, size);

    coalesce(bp);
}

/*
 * realloc - Naive implementation of realloc
 */
void *realloc(void *ptr, size_t size)
{
    size_t oldsize;
    void *newptr;

    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
        mm_free(ptr);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if(ptr == NULL) {
        return mm_malloc(size);
    }

    newptr = mm_malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if(!newptr) {
        return 0;
    }

    /* Copy the old data. */
    oldsize = GET_SIZE(HDRP(ptr));
    if(size < oldsize) oldsize = size;
    memcpy(newptr, ptr, oldsize);

    /* Free the old block. */
    mm_free(ptr);

    return newptr;
}

/* 
 * mm_checkheap - Check the heap for correctness. Helpful hint: You
 *                can call this function using mm_checkheap(__LINE__);
 *                to identify the line number of the call site.
 */
void mm_checkheap(int lineno)  
{ 
    lineno = lineno; /* keep gcc happy */
}

/* 
 * The remaining routines are internal helper routines 
 */

/* 
 * extend_heap - Extend heap with free block and return its block pointer
 */
static void *extend_heap(size_t words) 
{
    char *bp;
    size_t size, pralloc;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; 
    if ((long)(bp = mem_sbrk(size)) == -1)  
        return NULL;                                        

    pralloc = GET_PRALLOC(HDRP(bp));
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, pralloc, 0));         /* Free block header */   
    PUT(FTRP(bp), PACK(size, pralloc, 0));         /* Free block footer */   
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 0, 1));       /* New epilogue header */ 

    /* add new free block to free list */
    addFreeBLK(bp, size);
    /* Coalesce if the previous block was free */
    return coalesce(bp);                                          
}

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
static void *coalesce(void *bp) 
{
    size_t prev_alloc = GET_PRALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {            /* Case 1 */
        setNextPralloc(bp, 0);
        return bp;
    }

    else if (prev_alloc && !next_alloc) {      /* Case 2 */
        delBLK(bp);
        delBLK(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 1, 0));
        PUT(FTRP(bp), PACK(size, 1, 0));
        setNextPralloc(bp, 0);
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
        delBLK(bp);
        delBLK(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 1, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 1, 0));
        bp = PREV_BLKP(bp);
        setNextPralloc(bp, 0);
    }

    else {                                     /* Case 4 */
        delBLK(bp);
        delBLK(PREV_BLKP(bp));
        delBLK(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 1, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 1, 0));
        bp = PREV_BLKP(bp);
        setNextPralloc(bp, 0);
    }

    /* add new block to free list */
    addFreeBLK(bp, size);

    return bp;
}

/* 
 * place - Place block of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 */
static void *place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));   
    delBLK(bp);

    if ((csize - asize) >= (3 * DSIZE)) { 
        if(asize <= 256) {
            PUT(HDRP(bp), PACK(asize, 1, 1));
            // PUT(FTRP(bp), PACK(asize, 1));
            PUT(HDRP(NEXT_BLKP(bp)), PACK(csize-asize, 1, 0));
            PUT(FTRP(NEXT_BLKP(bp)), PACK(csize-asize, 1, 0));
            addFreeBLK(NEXT_BLKP(bp), csize - asize);
            // setNextPralloc()
        }
        else{
            PUT(HDRP(bp), PACK(csize - asize, 1, 0));
            PUT(FTRP(bp), PACK(csize - asize, 1, 0));
            PUT(HDRP(NEXT_BLKP(bp)), PACK(asize, 0, 1));
            // PUT(FTRP(NEXT_BLKP(bp)), PACK(asize, 1));
            addFreeBLK(bp, csize - asize);
            bp = NEXT_BLKP(bp);
            setNextPralloc(bp, 1);
        }
    }
    else { 
        PUT(HDRP(bp), PACK(csize, 1, 1));
        setNextPralloc(bp, 1);
        // PUT(FTRP(bp), PACK(csize, 1));
    }
    return bp;
}

/* 
 * find_fit - Find a fit for a block with asize bytes 
 */
static void *find_fit(size_t asize)
{
    int i = 0;
    size_t size = asize;
    void * bp;

    while(i < CLASSNUM) {
        if(((i == CLASSNUM - 1) || (size <= 1)) && (GET_FREE_LIST(i) != NULL)) {
            bp = GET_FREE_LIST(i);

            while((bp != NULL) && (asize > GET_SIZE(HDRP(bp)))) {
                bp = GET_PRED(bp);
            }

            if(bp != NULL) {
                return bp;
            }
        }
        size = size >> 1;
        i++;
    }
    return NULL;
}

/* 
 * addFreeBLK - add a free block to free list
 */
static void addFreeBLK(void *bp, size_t size){
    printf("bp=%p,  size=%ld\n", bp, size);
    int i = 0;
    // size_t tmp = size;
    for(; (i < CLASSNUM - 1) && (size > 1); i++) {
        size = size >> 1;  /* to *2 class */
    }
    // printf("i=%d\n", i);
    // size = tmp;
    void *curp = GET_FREE_LIST(i);
    void *succp = NULL;
    /* the class sorted from small to large */
    while( (curp != NULL) && (size > GET_SIZE(HDRP(curp)))) {
        succp = curp;
        curp = GET_PRED(curp);
    }
    /* no larger than bp */
    if (curp == NULL) {
        /* biggest, predest */
        if(succp != NULL){
            // printf("Case 1\n");
            printf("succ=%p    bp=%p\n", succp, bp);
            SET_PTR_VAL(pPRED(bp), NULL);
            SET_PTR_VAL(pSUCC(bp), succp);
            SET_PTR_VAL(pPRED(succp), bp);
        }
        /* no elements, first one */
        else{
           // printf("Case 2\n");
            SET_PTR_VAL(pPRED(bp), NULL);
            SET_PTR_VAL(pSUCC(bp), NULL);
            SET_FREE_LIST(i, bp);
        }
    }
    
    /* larger exist */
    else{
        /* succ < size <= curp */
        if(succp != NULL){
            // printf("Case 3\n");
            SET_PTR_VAL(pPRED(bp), curp);
            SET_PTR_VAL(pSUCC(bp), succp);
            SET_PTR_VAL(pSUCC(curp), bp);
            SET_PTR_VAL(pPRED(succp), bp);        
        }
        /* smallest */
        else{
            // printf("Case 4\n");
            SET_PTR_VAL(pPRED(bp), curp);
            SET_PTR_VAL(pSUCC(bp), NULL);
            SET_PTR_VAL(pSUCC(curp), bp);
            SET_FREE_LIST(i, bp);
        }
    }
    // printFreeList();
}

/*
 * delBLK - delete a block from free list
 */
static void delBLK(void *bp){
    int i = 0;
    size_t size = GET_SIZE(HDRP(bp));

    /* find which class bp in */
    for(; (i < CLASSNUM - 1) && (size > 1); i++){
        size = size >> 1;
    }
    /* if have pred */
    if(GET_PRED(bp) != NULL){
        /* if have succ */
        if(GET_SUCC(bp) != NULL){
            printf("succ=%p,  pred=%p\n", GET_SUCC(bp), GET_PRED(bp));
            SET_PTR_VAL(pPRED(GET_SUCC(bp)), GET_PRED(bp));
            SET_PTR_VAL(pSUCC(GET_PRED(bp)), GET_SUCC(bp));
        }
        /* no succ, smallest */
        else{
            SET_PTR_VAL(pSUCC(GET_PRED(bp)), NULL);
            SET_FREE_LIST(i, GET_PRED(bp));
        }
    }
    /* if no pred */
    else{
        /* if have succ, largest */
        if(GET_SUCC(bp) != NULL){
            SET_PTR_VAL(pPRED(GET_SUCC(bp)), NULL);
        }
        /* nos succ, the only one */
        else{
            SET_FREE_LIST(i, NULL);
        }
    }
} 

inline void setNextPralloc(void* bp, size_t pralloc){
    size_t size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
    size_t alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(size, pralloc, alloc));
} 

