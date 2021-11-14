



# Malloc Lab 记录

<p style="text-align:right;">by TomatoEater<br>2021年11月</p>

## 1. 准备

该实验主要是让我们模拟一个动态分配器，实现`mm_init`、`mm_malloc`、`mm_free`和`mm_realloc`函数。handout里提供了两个简单的验证文件`short1-bal.rep`和`short2-bal.rep`来测试我们算法的内存利用率和吞吐量，可以调用`./mdriver -f short1-bal.rep -V`来查看单个文件的测试结果。该课程的其他测试数据可以从[GitHub](https://link.zhihu.com/?target=https%3A//github.com/Ethan-Yan27/CSAPP-Labs/tree/master/yzf-malloclab-handout/traces)下载，得到一个`trace`文件夹，然后调用`./mdriver -t ./trace -V`来查看测试结果。

本实验只需要看书`Section 9.9`就可以完成了。书上给出了采用`first hit policy`隐式空闲链表的实现，而懒惰的我稍微改改抄抄就完成了`next fit policy`的隐式空闲链表。

## 2. 代码

```c
/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
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
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
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

/* Basic constants and macros */
#define WSIZE 4 /* Word and header/footer size (bytes) */
#define DSIZE 8 /* Double word size (bytes) */
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void *coalesce(void *bp);

static char *heap_listp;
static char *pre_listp; /* next fit policy */

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0)); /* Free block header */
    PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0); /* Alignment padding */
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3*WSIZE), PACK(0, 1)); /* Epilogue header */
    heap_listp += (2*WSIZE);
    pre_listp = heap_listp;
    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}


static void *find_fit(size_t asize)
{
    void *bp;

    for (bp = pre_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
            return bp;
    }
    for (bp = heap_listp; bp != pre_listp; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
            return bp;
    }
    return NULL;
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    
    if ((csize - asize) >= (2*DSIZE)) { /* split */
        PUT(HDRP(bp),PACK(asize,1));
        PUT(FTRP(bp),PACK(asize,1));
        PUT(HDRP(NEXT_BLKP(bp)),PACK(csize - asize,0));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(csize - asize,0));
    }
    else {
        PUT(HDRP(bp),PACK(csize,1));
        PUT(FTRP(bp),PACK(csize,1));
    }
    pre_listp = bp;
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize; /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;
    
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;
    
    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {             /* Case 1 */ 
        pre_listp = bp;
        return bp;
    }

    else if (prev_alloc && !next_alloc) {       /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
    }

    else if (!prev_alloc && next_alloc) {       /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        
    }

    else {                                      /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    pre_listp = bp;
    return bp;
}
/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
        return mm_malloc(size);
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    void *newptr;
    size_t newSize, oldSize;
    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    oldSize = GET_SIZE(HDRP(ptr));
    newSize = GET_SIZE(HDRP(newptr));
    if (oldSize < newSize)
        newSize = oldSize;
    memcpy(newptr, ptr, newSize - DSIZE);
    mm_free(ptr);
    return newptr;
}
```

事实上只需要写`mm_realloc`函数以及修改`find_fit`就可以了。其他函数要么在文本中有，要么在课后练习的答案中有。

## 3. 测试

```bash
Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   90%    5694  0.001195  4766
 1       yes   93%    5848  0.000806  7257
 2       yes   94%    6648  0.002441  2724
 3       yes   96%    5380  0.002535  2122
 4       yes   66%   14400  0.000097148607
 5       yes   89%    4800  0.002055  2336
 6       yes   87%    4800  0.002005  2394
 7       yes   55%   12000  0.013150   913
 8       yes   51%   24000  0.006653  3608
 9       yes   26%   14401  0.081437   177
10       yes   34%   14401  0.001959  7349
Total          71%  112372  0.114333   983

Perf index = 43 (util) + 40 (thru) = 83/100
```

trace9和trace10测试了`mm_realloc`函数，可见`mm_realloc`函数的空间利用率（util）相当的低。这是因为我们的实现相当的naive。如果想要提高空间利用率，可以realloc时先尝试合并前后空闲块，并尝试在原先的位置放置数据。但是懒惰的我不想再为此写一个coalesce函数了。

## 4. 就这？

对书上代码的稍微修改就可以达到83/100的分数，此时可能只用了半天不到的时间。但是这个lab其实远不只如此。malloclab的实现还有以下几种选择：

- 显式空闲链表：将所有的free block数链接起来。缩短allocation time，但是会增加内部碎片。
- 分离适配：维护多个链表，每个链表中的块大小都在一定的范围之内，不同链表的块的大小范围不同。相当麻烦的编程，优秀的空间利用率和速度。GNU中的malloc就采用该思路。如果按该思路实现该lab可以得到满分，只不过消失了一周的时间。
- 伙伴系统：块大小均为2的幂次方。不错的寻找和合并速度。但是造成很多内部碎片，不适合通常意义下的allocator。不过对于一个频繁申请2的幂次方块的程序来说，伙伴系统将是不错的选择。

个人认为如果想要实现分离适配的话，应该算是所有实验中编程量和编程难度最大的一个lab了。

## 5. 写在后面

本人于2021/11/14水完了malloc lab，用时一天左右。malloc lab其实相当有挑战性，只不过懒惰的我明白放弃其实是种智慧。最后一个poxy lab应该不会做了，套接字可能就直接上自顶向下了。可能本篇作为所有lab的收尾有点心不甘，但是又何妨。最后以csapp官网的一句话收尾malloc lab：

> One of our favorite labs. When students finish this one, they really understand pointers!