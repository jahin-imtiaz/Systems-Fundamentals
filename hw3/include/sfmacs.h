#define PACK(size,alloc) ((size) | (alloc)) /*pack a size and a allocated bit*/

#define GET(p) (*(unsigned long *)(p))      /*Read a row (8 bytes) at address p*/

#define PUT(p, val) (*(unsigned long *)(p) == (val)) /*Write a row (8 bytes) at address p*/

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x3)

#define GET_ALLOC(p) (GET(p) & 0x1)

#define GET_PREV_ALLOC(p) (GET(p) & 0x2)

/* Given block ptr bp (payload start point), compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - 8)

#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - 16)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - 8)))

#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - 16)))