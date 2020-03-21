/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include <errno.h>
#include <limits.h>
#include "sfmacs.h"

size_t get_required_block_bize(size_t size);
void set_heap();
void add_wilderness_block(sf_block *block);
void add_free_block(sf_block *block);
void free_list_heads_init();
void free_list_helper_init();
sf_block *find_fit(size_t size);
void split_block(sf_block *bp, size_t size);
int find_free_list_index(size_t msize);

char *my_heap;
char *prologue_header;
char *epilogue_header;
size_t sf_free_list_helper[NUM_FREE_LISTS];

void *sf_malloc(size_t size) {
    sf_block *bp;
    //Set up heap, prologue and epilogue for the first call
    if(my_heap == NULL){
        //NULL when no memory left or any error occured
        if((my_heap = (char *)sf_mem_grow()) == NULL){
            return NULL;
        }
        else{

            free_list_heads_init();     //initialize the free list

            free_list_helper_init();    //initialize the free list helper

            set_heap();     //set up heap and add unused block in free list
        }
    }

    size_t blocksize;

    //verify the size
    if((int) size == 0){
        return NULL;
    }

    //check if the size is too big to allocate (due to limited MAX blocksize)
    if(size > (ULONG_MAX -8)){
        return NULL;
    }

    //calculate the blocksize
    blocksize = get_required_block_bize(size);

    //check if the required blocksize is greater than max value of unsigned long
    if (size >= blocksize){
        //blocksize will be less than the size if rounding up resulted in a roll over
        return NULL;
    }

    //search free list for a block
    if((bp = find_fit(blocksize)) != NULL){

        split_block(bp, blocksize);     //split block if free block is too big
        return (*bp).body.payload;      //return pointer to the payload section.
    }

    //No block found. extend memory and place the block
        //call mem_grow
        //coalesce the new memory
        //check if memory is full
        //if not, check if the size is enough
        //if not, do this while size is not enough
        //split the block
        //put the remaining block in the wilderness block
}

void sf_free(void *ptr) {
    return;
}

void *sf_realloc(void *ptr, size_t size) {
    return NULL;
}

void *sf_memalign(size_t align, size_t size) {
    return NULL;
}

//calculates the required block size
size_t get_required_block_bize(size_t size){
    size = size + 8;
    size = ((size+63) /64) * 64;
    return size;
}

//add prologue and epilogue in heap
void set_heap(){
    prologue_header = my_heap +56;                  //get to the point

    PUT(prologue_header, PACK(64,3));               //set the value

    PUT(prologue_header+56, PACK(64,3));            //set the footer

    epilogue_header = ((char *)sf_mem_end()) - 8;   //get the last address from sf_mem_end and go back 8 bytes

    PUT(epilogue_header, PACK(0, 1));               //set the epilogue header

    //create free block out of the remaining memory
    int s = epilogue_header - (prologue_header + 64);  //size is the number of bytes in between prologue footer and epiloge header
    PUT(prologue_header+64, PACK(s, 2));               //set the header
    PUT(epilogue_header-8,  PACK(s, 2));               //set the footer

    //put the remaining block in the free list
    add_wilderness_block((sf_block *)(prologue_header+56));
}

//initialize the free list
void free_list_heads_init(){
    int i;
    for(i = 0; i < NUM_FREE_LISTS; i++){
        sf_block sentinel;
        sentinel.body.links.next = &sentinel;
        sentinel.body.links.prev = &sentinel;
        sf_free_list_heads[i] =sentinel;
    }
}

//initialize the free list helper with fiboncacci numbers
void free_list_helper_init(){
    sf_free_list_helper[0] = 1;
    sf_free_list_helper[1] = 2;

    size_t i;
    for(i = 2; i < NUM_FREE_LISTS - 1; i++){
        sf_free_list_helper[i] = sf_free_list_helper[i-1] + sf_free_list_helper[i-2];
    }
}

//find free block in the list
//returns a pointer to the block
sf_block *find_fit(size_t size){

    int i;
    sf_block *ptr;
    for(i = 0; i < NUM_FREE_LISTS; i++){
        ptr = &(sf_free_list_heads[i]);
        //loop through each list
        while(ptr->body.links.next != ptr){

            //check the size of this block and return the block if it is equal
            //or bigger than the required size
            if(size <= GET_SIZE(((char *)(ptr->body.links.next)) + 8)){

                //remove the block from the list;
                sf_block *tmp = (ptr->body.links.next);
                (tmp->body.links.prev)->body.links.next = tmp->body.links.next;
                (tmp->body.links.next)->body.links.prev = tmp->body.links.prev;
                return tmp;
            }
            ptr = ptr->body.links.next ;
        }
    }
    //no big enough block is found
    return NULL;
}

void split_block(sf_block *bp, size_t size){

    size_t bsize = GET_SIZE((char *)bp + 8);

    if((bsize-size) >= 64){ //split
        PUT((char *)bp + 8, PACK(size, 1)); //set allocator bit for first block header

        PUT(((char *)bp + (size+8)), PACK(bsize-size, 2)); //set header for new block with alloc bit(0) and prev_alloc bit(1)

        char *tmp = ((char *)bp +bsize);  //move pointer to the footer of the new block

        PUT(tmp, PACK(bsize-size, 2));    //set footer with same value

        *((size_t *)(tmp + 8)) = SET_PREV_ALLOC_ZERO(tmp+8);    //change prev_alloc bit for next block in heap to 0
        //insert new free block in the free list
        tmp = (char *)bp + size;//move tmp pointer to point to the new block
        add_free_block((sf_block *)tmp);
    }
    else{//don't split
        PUT((char *) bp + 8, PACK(bsize, 1));       //change header allocator bit
        PUT(((char *)bp + (bsize+8)), PACK(GET_SIZE(((char *)bp + (bsize+8))), 2));//change prev_alloc bit for next block in heap
    }

}

//add the wilderness block in the free list
void add_wilderness_block(sf_block *block){
    (*block).body.links.next = sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next;

    (*block).body.links.prev = &sf_free_list_heads[NUM_FREE_LISTS-1];

    (*(*block).body.links.next).body.links.prev = block;

    sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next = block;
}


//add a free block in the free list
void add_free_block(sf_block *block){

    size_t msize = GET_SIZE((char *)block + 8);//get the size of the block
    msize = msize / 64;     //turn it into m value
    int index = find_free_list_index(msize);//find the appropriate size class for that block

    //insert it in the list
    (*block).body.links.next = sf_free_list_heads[index].body.links.next;

    (*block).body.links.prev = &sf_free_list_heads[index];

    (*(*block).body.links.next).body.links.prev = block;

    sf_free_list_heads[index].body.links.next = block;
}

//find the appropriate size class given the MSIZE and return the index
int find_free_list_index(size_t msize){

    if( msize > 0 && msize <= 3){   //return (msize-1) if it is 1,2 or 3
        return (int) (msize -1);
    }
    else{   //else return index of appropriate size class
        int i;
        for(i = 3; i <= (NUM_FREE_LISTS - 3); i++){
            if((msize > sf_free_list_helper[i-1]) && (msize <= sf_free_list_helper[i])){
                return i;
            }
        }
        //for all block that are too big to fit into any size class, put them in
        //second to last list of the free list
        return NUM_FREE_LISTS-2;
    }
}

sf_block *coalesce(sf_block *block){

    size_t size = GET_SIZE((char *)block + 8);
    size_t prev_alloc = GET_PREV_ALLOC((char *)block + 8);
    size_t next_alloc = GET_ALLOC((char *)block + (size + 8));

    if(prev_alloc && next_alloc){               //prev and next block is allocated
        return block;
    }
    else if(prev_alloc && !next_alloc){          //prev block is allocated and next block is free

        //remove next block from the free list
        sf_block *tmp = (sf_block *)((char *)block + size);
        (tmp->body.links.prev)->body.links.next = tmp->body.links.next;
        (tmp->body.links.next)->body.links.prev = tmp->body.links.prev;

        //update the new size
        size += GET_SIZE((char *)tmp +8);

        //update header for the current block
        PUT((char *)block + 8, PACK(size,2)); //set prev alloc bit to 1
        *((size_t *) ((char *)block + 8)) = SET_ALLOC_ZERO((char*)block +8);//set alloc bit to 0

        //update footer for the current block
        PUT((char *)block + GET_SIZE((char *)block +8) , PACK(size, 2));
        *((size_t *) ((char *)block +GET_SIZE((char *)block +8))) = SET_ALLOC_ZERO((char *)block + GET_SIZE((char *)block +8));

        return block;

    }
    else if(!prev_alloc && next_alloc){          //prev block is free and next block is allocated

        //get prev blocks size
        size_t psize = GET_SIZE((char *)block);

        //remove prev block from the free list
        sf_block *tmp = (sf_block *)((char *)block - psize);
        (tmp->body.links.prev)->body.links.next = tmp->body.links.next;
        (tmp->body.links.next)->body.links.prev = tmp->body.links.prev;

        //update the new header
        *((size_t *)((char *)tmp +8)) = *((size_t *)((char *)tmp +8)) + size;

        //update footer for the current block
        *((size_t *)((char *)block + size)) = *((size_t *)((char *)tmp +8)) + size;

        return tmp;

    }
    else{                                        //prev and next block is free

        //remove next block from the list
        sf_block *tmp_next = (sf_block *)((char *)block + size);
        (tmp_next->body.links.prev)->body.links.next = tmp_next->body.links.next;
        (tmp_next->body.links.next)->body.links.prev = tmp_next->body.links.prev;

        //get next block size
        size_t nsize = GET_SIZE((char *)tmp_next +8);
        //get prev blocks size
        size_t psize = GET_SIZE((char *)block);

        //remove prev block from the list
        sf_block *tmp_prev = (sf_block *)((char *)block - psize);
        (tmp_prev->body.links.prev)->body.links.next = tmp_prev->body.links.next;
        (tmp_prev->body.links.next)->body.links.prev = tmp_prev->body.links.prev;

        //update the header for the prev block
        *((size_t *)((char *)tmp_prev +8)) = *((size_t *)((char *)tmp_prev +8)) + size + nsize;

        //update the footer for the next block
        *((size_t *)((char *)tmp_next + nsize)) = *((size_t *)((char *)tmp_prev +8)) + size + nsize;

        return tmp_prev;

    }
}