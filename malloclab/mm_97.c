/* 
 * 孙少凡 2100013085@stu.pku.edu.cn
 *
 * 采用分离空闲链表
 * heap开头是CLASSNUM * DSIZE的空间，用于存放空闲链表的表头（初始内容为NULL）
 * 序言块和结尾块结构与书本上的一致， 普通块结构如下（最小为16B）：
 * -----------------------------------------------------
 * | hdr(4B) | pred(4B) | succ(4B) | content | ftr(4B) |
 * -----------------------------------------------------
 * 其中，空闲块有脚部，而已分配块没有。头部第二位额外储存了前一个块的分配标志
 * pred和succ并非真正地址，是相对heap_listp的偏移量，NULL的读写需要特判
 * 同类中的空闲块归类时直接插入表头，搜索策略为分离适配
 * 
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

/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ 
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */  

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))            
#define PUT(p, val)  (*(unsigned int *)(p) = (val))    

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)                   
#define GET_ALLOC(p) (GET(p) & 0x1)                    
#define GET_PREV_ALLOC(p) (GET(p) & 0x2)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) 
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) 

/* pointer to pointer to pred/succ */
#define pPRED(bp)  ((char *)(bp))
#define pSUCC(bp)  ((char *)(bp) + WSIZE)

/* Offset of pointer to pred/succ */
#define GET_PRED_L(bp)  (char *)((unsigned long)(heap_listp) + (unsigned long)GET(bp))
#define GET_SUCC_L(bp)  (char *)((unsigned long)(heap_listp) + (unsigned long)GET((char *)(bp) + WSIZE))

/* Set address p to the offset of a pointer (4 bytes) */
#define SET_PTR_VAL_L(p, ptr)  (PUT(p, (unsigned int)((unsigned long)(ptr) - (unsigned long)(heap_listp))))

/* Read and write the i_th size-class's first pointer */
#define GET_FREE_LIST(i)      (*((free_list) + (i)))
#define SET_FREE_LIST(i, bp)  ((*((free_list) + (i))) = (char *)(bp))

/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */  
static char **free_list;      /* free list */
static char *heap_listp_copy = 0;  /* a copy for examine consistency */
static char **free_list_copy; 
#define CLASSNUM 16           /* numbers of classes in free list */

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void *place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void addFreeBLK(void *bp, size_t size);
static void delBLK(void *bp);
static inline void SET_PTR_VAL(void* p, void *ptr){
    if(ptr == NULL) {
        PUT(p, 0);
        return;
    }
    SET_PTR_VAL_L(p, ptr);
}
static inline void* GET_PRED(void *bp){
    if(GET(bp) == 0) return NULL;
    return GET_PRED_L(bp);
}
static inline void* GET_SUCC(void*bp){
    if(GET((char*)(bp) + WSIZE) == 0) return NULL;
    return GET_SUCC_L(bp);
}
static size_t checkheap();
static size_t checkfreelist();
static size_t checkblock(void *bp);
/* 
 * mm_init - Initialize the memory manager 
 */
int mm_init(void) 
{
    /* allocate space for and initialize free_list first */
    if ((free_list = mem_sbrk(CLASSNUM * sizeof(char *))) == (void *)-1) 
        return -1;  
    for(int i = 0; i < CLASSNUM; i++) SET_FREE_LIST(i, NULL);
    free_list_copy = free_list;

    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) 
        return -1;
    PUT(heap_listp, 0);                          /* Alignment padding */
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 3)); /* Prologue header */ 
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 3)); /* Prologue footer */ 
    PUT(heap_listp + (3*WSIZE), PACK(0, 3));     /* Epilogue header */
    heap_listp += (2*WSIZE); 
    heap_listp_copy = heap_listp;                    

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(1 << 6) == NULL)  /* initialized by this size can save space */
        return -1;
    return 0;
}

/* 
 * malloc - Allocate a block with at least size bytes of payload 
 */
void *malloc(size_t size) 
{
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
    if (size <= WSIZE)                                          
        asize = 2 * DSIZE;                                        
    else
        asize = DSIZE * ((size + (WSIZE) + (DSIZE-1)) / DSIZE); 

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
    if (bp == 0) 
        return;

    if (heap_listp == 0){
        mm_init();
    }

    size_t size = GET_SIZE(HDRP(bp));
    size_t prevalloc = GET_PREV_ALLOC(HDRP(bp));

    /* set hdr and ftr */
    PUT(HDRP(bp), PACK(size, (prevalloc | 0)));
    PUT(FTRP(bp), PACK(size, (prevalloc | 0)));

    /* add to free list */
    addFreeBLK(bp, size);

    /* merge free block */
    coalesce(bp);

    /* check heap */
    // mm_checkheap(3);
}

/*
 * realloc - Naive implementation of realloc
 */
void *realloc(void *ptr, size_t size)
{
    size_t oldsize;
    void *newptr;
    size_t asize; 

    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
        mm_free(ptr);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if(ptr == NULL) {
        return mm_malloc(size);
    }

    oldsize = GET_SIZE(HDRP(ptr));

    /* Alignment */
    if(size <= WSIZE) {
        asize = 2 * DSIZE;
    }
    else {
        asize = DSIZE * ((size + (WSIZE) + (DSIZE - 1)) / DSIZE);
    }

    /* if enough, return directly */
    if(oldsize >= asize) {
        return ptr;
    }

    /* not enough, reallocate space */
    newptr = mm_malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if(!newptr) {
        return 0;
    }

    /* Copy the old data. */
    if(size < oldsize) oldsize = size;
    memcpy(newptr, ptr, oldsize);

    /* Free the old block. */
    mm_free(ptr);

    return newptr;
}

/*
 * calloc - Naive implementation of calloc
 */
void *calloc (size_t nmemb, size_t size){
    /* an error, return */
    if(nmemb <= 0 || size <= 0) return NULL;

    size_t asize = nmemb * size;
    void *ptr;

    /* Alignment */
    if(asize <= WSIZE) {
        asize = 2 * DSIZE;
    }
    else{
        asize = DSIZE * ((asize + (WSIZE) + (DSIZE - 1)) / DSIZE);
    }

    /* allocate new space */
    ptr = mm_malloc(asize);
    if(!ptr){
        return NULL;
    }

    /* set 0 */
    memset(ptr, 0, asize);
    return ptr;
}

/* 
 * mm_checkheap - Check the heap for correctness. 
 */
void mm_checkheap(int select)  
{ 
    /* check global variables */
    if(heap_listp != heap_listp_copy) {
        printf("ERROR: heap_listp has been changed from (%p) to (%p)\n", 
                heap_listp_copy, heap_listp);
    }
    if(free_list != free_list_copy) {
        printf("ERROR: free_list has been changed from (%p) to (%p)\n", 
                free_list, free_list_copy);
    }

    /* end check */
    if(select == 0) return;

    /* only check heap */
    if(select == 1) {
        checkheap();
        return;
    }

    /* only check list */
    if(select == 2) {
        checkfreelist();
        return;
    }

    /* check heap and list and if free block number match */
    else{
        size_t heapfreeblock = checkheap();
        size_t listfreeblock = checkfreelist();
        if(heapfreeblock != listfreeblock){
            printf("ERROR: Different number of free blocks in heap(%ld) and list(%ld)\n", 
                    heapfreeblock, listfreeblock);
        }
        return;
    }
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
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; 
    if ((long)(bp = mem_sbrk(size)) == -1)  
        return NULL;                                        

    size_t prevalloc = GET_PREV_ALLOC(HDRP(bp));
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, (prevalloc | 0)));   /* Free block header */   
    PUT(FTRP(bp), PACK(size, (prevalloc | 0)));   /* Free block footer */   
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));         /* New epilogue header */ 

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
    /* prev may be used, may have no footer */
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {            /* Case 1 */
        /* cannot merge, set next block's prealloc bit, return */
        size_t nsize = GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(nsize, (0 | next_alloc)));
        return bp;
    }

    else if (prev_alloc && (!next_alloc)) {      /* Case 2 */
        delBLK(bp);
        delBLK(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 2));
        PUT(FTRP(bp), PACK(size, 2));
    }

    else if ((!prev_alloc) && next_alloc) {      /* Case 3 */
        delBLK(bp);
        delBLK(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 2));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 2));
        bp = PREV_BLKP(bp);
    }

    else {                                     /* Case 4 */
        delBLK(bp);
        delBLK(PREV_BLKP(bp));
        delBLK(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 2));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 2));
        bp = PREV_BLKP(bp);
    }

    /* set next block's prevalloc bit */
    size_t nsize = GET_SIZE(HDRP(NEXT_BLKP(bp)));
    size_t nalloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(nsize, (0 | nalloc)));

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
    // printf("call delBLK from place, bp=%p\n", bp);
    delBLK(bp);

    /* free block is too big, split */
    if ((csize - asize) >= (2 * DSIZE)) { 
        /* put small block ahead, save a little space */
        /* the number is defined through experiments */
        if(asize <= 256) {
            PUT(HDRP(bp), PACK(asize, 3));

            PUT(HDRP(NEXT_BLKP(bp)), PACK(csize-asize, 2));
            PUT(FTRP(NEXT_BLKP(bp)), PACK(csize-asize, 2));
            /* next block's prevalloc is still 0 */

            addFreeBLK(NEXT_BLKP(bp), csize - asize);
        }
        else{
            PUT(HDRP(bp), PACK(csize - asize, 2));
            PUT(FTRP(bp), PACK(csize - asize, 2));

            PUT(HDRP(NEXT_BLKP(bp)), PACK(asize, 1));

            addFreeBLK(bp, csize - asize);

            bp = NEXT_BLKP(bp);

            /* set next block's prealloc bit */
            size_t nsize = GET_SIZE(HDRP(NEXT_BLKP(bp)));
            size_t nalloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
            PUT(HDRP(NEXT_BLKP(bp)), PACK(nsize, (2 | nalloc)));
        }
    }

    /* remainder too small, allocate all */
    else { 
        PUT(HDRP(bp), PACK(csize, 3));

        /* set next block's prealloc bit */
        size_t nsize = GET_SIZE(HDRP(NEXT_BLKP(bp)));
        size_t nalloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(nsize, (2 | nalloc)));
    }

    return bp;
}

/* 
 * find_fit - Find a fit for a block with asize bytes, segregated fit 
 */
static void *find_fit(size_t asize)
{
    int i = 0;
    size_t size = asize;
    void * bp = NULL;

    while(i < CLASSNUM) {
        if(((i == CLASSNUM - 1) || (size <= 1)) && (GET_FREE_LIST(i) != NULL)) {
            bp = GET_FREE_LIST(i);
            while((bp != NULL) && (asize > GET_SIZE(HDRP(bp)))) {
                bp = GET_PRED(bp);
            }
            if(bp != NULL){
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
    int i = 0;
    /* now is inserting into head directly */
    /* if sorted by size, remove the "//" before and after the for loop */
    // size_t tmp = size;
    for(; (i < CLASSNUM - 1) && (size > 1); i++) {
        size = size >> 1;  /* to *2 class */
    }
    // size = tmp;
    void *curp = GET_FREE_LIST(i);
    void *succp = NULL;

    /* if use real size, the class list sorted from small to large */
    /* otherwise, size <= 1, put at the head of the class list */
    while( (curp != NULL) && (size > GET_SIZE(HDRP(curp)))) {
        succp = curp;
        curp = GET_PRED(curp);
    }

    /* no larger than bp */
    if (curp == NULL) {
        /* biggest, predest */
        if(succp != NULL){
            SET_PTR_VAL(pPRED(bp), NULL);
            SET_PTR_VAL(pSUCC(bp), succp);
            SET_PTR_VAL(pPRED(succp), bp);
        }
        /* no elements, first one */
        else{
            SET_PTR_VAL(pPRED(bp), NULL);
            SET_PTR_VAL(pSUCC(bp), NULL);
            SET_FREE_LIST(i, bp);
        }
    }
    
    /* larger exist */
    else{
        /* succ < size <= curp */
        if(succp != NULL){
            SET_PTR_VAL(pPRED(bp), curp);
            SET_PTR_VAL(pSUCC(bp), succp);
            SET_PTR_VAL(pSUCC(curp), bp);
            SET_PTR_VAL(pPRED(succp), bp);        
        }
        /* smallest */
        else{
            SET_PTR_VAL(pPRED(bp), curp);
            SET_PTR_VAL(pSUCC(bp), NULL);
            SET_PTR_VAL(pSUCC(curp), bp);
            SET_FREE_LIST(i, bp);
        }
    }
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

/*
 * checkheap - check on heap level, return free block number in heap
 */
static size_t checkheap(){
    size_t free_block = 0;

    char *bp = heap_listp;
    if((GET_SIZE(HDRP(heap_listp)) != DSIZE) || (!GET_ALLOC(HDRP(heap_listp)))) {
        printf("ERROR: Garbled prologue header!\n");
    }
    checkblock(bp);

    /* check blocks and whether consecutive free blocks */
    size_t prev_alloc = 1;
    for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        size_t cur_alloc = checkblock(bp);
        /* if prev block and cur block both unallocated */
        if((!prev_alloc) && (!cur_alloc)) {
            printf("WARNING: Consecutive free blocks at (%p) and prev!\n", bp);
        }
        /* count free blocks */
        if(!cur_alloc) {
            free_block++;
        }
    }

    /* if normal, now bp is at the epilogue */
    if((GET_SIZE(HDRP(bp)) != 0) || (!GET_ALLOC(HDRP(bp)))) {
        printf("ERROR: Garbled epilogue header!\n");
    }
    return free_block;
}

/*
 * checkfreelist - check on list level, return free block number in list
 */
static size_t checkfreelist() {
    char *bp;
    size_t free_block = 0;
    for(size_t i = 0; i < CLASSNUM; i++) {
        bp = GET_FREE_LIST(i);
        char *succp = NULL;
        for(; bp != NULL; bp = GET_PRED(bp)){
            /* if the address legal */
            if(((size_t)bp < (size_t)mem_heap_lo()) || ((size_t)bp > (size_t)mem_heap_hi())) {
                printf("ERROR: illegal address (%p)!\n", bp);
            }
            /* if connections between blocks match */
            if(GET_SUCC(bp) != succp){
                printf("ERROR: Pred and succ not match between (%p) and (%p)!\n", succp, bp);
            }
            /* if the block is unallocated */
            size_t hdalloc = GET_ALLOC(HDRP(bp));
            if(hdalloc == 1) {
                printf("ERROR: Allocated block at (%p) in free list!\n", bp);
            }
            /* count free block */
            else{
                free_block++;
            }
            /* if size is correct */
            size_t hdsize = GET_SIZE(HDRP(bp));
            size_t j = 0;
            for(; (j < CLASSNUM - 1) && (hdsize > 1); j++) {
                hdsize = hdsize >> 1;
            }
            if(i != j) {
                printf("ERROR: Incorrect size class: size=%d(should in class %ld) in class %ld\n", 
                        GET_SIZE(HDRP(bp)), j, i);
            }
            succp = bp;
        }
    }
    return free_block;
}

/*
 * checkblock - check on block level, return if the block is allocated 
 */
static size_t checkblock(void *bp){
    /* whether bp is aligned */
    if((unsigned long)bp % 8 != 0) {
        printf("ERROR: Pointer (%p) is not aligned!\n", bp);
    }
    size_t size = GET_SIZE(HDRP(bp));
    /* whether block size is aligned */
    if(size % 8 != 0) {
        printf("ERROR: Payload size at (%p) is not aligned!\n", bp);
    }
    size_t hdalloc = GET_ALLOC(HDRP(bp));
    /* unallocated block, examine hdr and ftr */
    if(hdalloc == 0) {
        size_t ftalloc = GET_ALLOC(HDRP(bp));
        size_t ftsize = GET_SIZE(HDRP(bp));
        if((hdalloc != ftalloc) || (size != ftsize)){
            printf("ERROR: Block at (%p) HDR doesn't match FTR!\n", bp);
        }
    }
    return hdalloc;
}


