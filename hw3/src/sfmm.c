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
sf_block *coalesce(sf_block *block);
int valid_pointer(void *ptr);
int is_wilderness(sf_block *block);

char *my_heap;
char *prologue_header;
char *epilogue_header;
size_t sf_free_list_helper[NUM_FREE_LISTS];
int split_wilderness_flag = 0;

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
    char *prev_end_point;
    while((prev_end_point = (char *)sf_mem_grow()) != NULL){

        //move the epilogue header
        epilogue_header = (char *)sf_mem_end - 8;
        *(size_t *)epilogue_header = *(size_t *)(prev_end_point - 8);

        //create a new block from the remaining
        int s = epilogue_header - (prev_end_point -8);          //size is the number of bytes in between prologue footer and epiloge header
        size_t prev_alloc = GET_PREV_ALLOC(epilogue_header);    //check if the last block was allocated or not
        PUT(prev_end_point-8, PACK(s, prev_alloc));             //set the header
        PUT(epilogue_header-8,  PACK(s, prev_alloc));           //set the footer

        //coalesce the new block
        sf_block *coalesced_block = coalesce((sf_block *)(prev_end_point-16));

        //put the new block in the wilderness block
        add_wilderness_block(coalesced_block);

        //search free list for a block
        if((bp = find_fit(blocksize)) != NULL){

            split_block(bp, blocksize);     //split block if free block is too big
            return (*bp).body.payload;      //return pointer to the payload section.
        }

    }
    return NULL;

}

void sf_free(void *ptr) {       //ptr is pointer to the payload section or next address after header
    if(valid_pointer(ptr)){

        PUT(HDRP(ptr), SET_ALLOC_ZERO(HDRP(ptr)));  //update header (set alloc bit to 0)

        PUT(FTRP(ptr), SET_ALLOC_ZERO(HDRP(ptr)));  //update footer (set alloc bit to 0)

        sf_block *coalesced_block =  coalesce((sf_block *)((char *)ptr - 16));   //coalesce the block

        //check if its the wilderness block
        if(is_wilderness(coalesced_block)){      //if yes, add it to the wilderness list
            add_wilderness_block(coalesced_block);
        }
        else{   //if no, update the header of the next block (set prev_alloc bit to 0). Then, add it to an appropriate size list
            PUT(HDRP(NEXT_BLKP((char *)coalesced_block + 16)), SET_PREV_ALLOC_ZERO(HDRP(NEXT_BLKP((char *)coalesced_block + 16))));
            add_free_block(coalesced_block);
        }
    }
    else{
        abort();
    }
    return;
}

void *sf_realloc(void *ptr, size_t size) {
    return NULL;
}

void *sf_memalign(size_t size, size_t align) {
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

    return;
}

//initialize the free list
void free_list_heads_init(){
    int i;
    for(i = 0; i < NUM_FREE_LISTS; i++){

        sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
        sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
    }
    return;
}

//initialize the free list helper with fiboncacci numbers
void free_list_helper_init(){
    sf_free_list_helper[0] = 1;
    sf_free_list_helper[1] = 2;

    size_t i;
    for(i = 2; i < NUM_FREE_LISTS - 1; i++){
        sf_free_list_helper[i] = sf_free_list_helper[i-1] + sf_free_list_helper[i-2];
    }
    return;
}

//find free block in the list
//returns a pointer to the block
sf_block *find_fit(size_t size){

    int i;
    sf_block *ptr;
    sf_block *head;
    for(i = 0; i < NUM_FREE_LISTS; i++){
        ptr = (sf_free_list_heads[i]).body.links.next;
        head = (sf_free_list_heads[i]).body.links.next;
        //loop through each list
        while(ptr->body.links.next != head){

            //check the size of this block and return the block if it is equal
            //or bigger than the required size
            if(size <= GET_SIZE((char *)ptr + 8)){

                //set the split flag if it is the wilderness list
                if(i == NUM_FREE_LISTS-1){
                    split_wilderness_flag = 1;
                }

                //remove the block from the list;
                (ptr->body.links.prev)->body.links.next = ptr->body.links.next;
                (ptr->body.links.next)->body.links.prev = ptr->body.links.prev;
                return ptr;
            }
            ptr = ptr->body.links.next ;
        }
    }
    //no big enough block is found
    return NULL;
}

//Splits a block if too big
void split_block(sf_block *bp, size_t size){

    size_t bsize = GET_SIZE((char *)bp + 8);

    if((bsize-size) >= 64){ //split
        size_t prev_alloc = GET_PREV_ALLOC((char *)bp + 8);
        *((size_t *)((char *)bp + 8)) = (size | prev_alloc) | 1;    //set allocator bit and prev alloc bit for first block header

        *((size_t *)((char *)bp + (size+8))) = (bsize-size) | 2;    //set header for new block with alloc bit(0) and prev_alloc bit(1)

        char *tmp = ((char *)bp +bsize);  //move pointer to the footer of the new block

        *(size_t *)tmp = *((size_t *)((char *)bp + (size+8)));  //set footer with same value

        //insert new free block in the free list
        tmp = (char *)bp + size;        //move tmp pointer to point to the new block
        if(split_wilderness_flag == 1){
            split_wilderness_flag = 0;
            add_wilderness_block((sf_block *)tmp);
        }
        else{
            add_free_block((sf_block *)tmp);
        }
    }
    else{//don't split
        *((size_t *)((char *)bp + 8)) = (*((size_t *)((char *)bp + 8))) | 1;     //change header allocator bit to 1
        *((size_t *)((char *)bp + (bsize+8))) = (*((size_t *)((char *)bp + (bsize+8)))) | 2;//change prev_alloc bit for next block in heap
    }

    return;

}

//add the wilderness block in the free list
void add_wilderness_block(sf_block *block){

    block->body.links.next = &sf_free_list_heads[NUM_FREE_LISTS-1];

    block->body.links.prev = &sf_free_list_heads[NUM_FREE_LISTS-1];

    sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next = block;

    sf_free_list_heads[NUM_FREE_LISTS-1].body.links.prev = block;

    return;
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

    return;
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

//coalesce free blocks
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

        //update header for the current block
        *((size_t *) ((char *)block + 8)) = *((size_t *) ((char *)block + 8)) + GET_SIZE((char *)tmp +8);   //update the new size

        //update footer for the current block
        *((size_t *) ((char *)block +GET_SIZE((char *)block +8))) = *((size_t *) ((char *)block + 8));

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
        *((size_t *)((char *)block + size)) = *((size_t *)((char *)tmp +8));

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
        *((size_t *)((char *)tmp_next + nsize)) = *((size_t *)((char *)tmp_prev +8));

        return tmp_prev;

    }
}

//return 1 if valid pointer, return 0 otherwise
int valid_pointer(void *ptr){
    if(ptr == NULL){    //pointer is NULL
        return 0;
    }

    size_t alloc = GET_ALLOC(HDRP(ptr));
    if(alloc == 0){     //alloc bit 0
        return 0;
    }

    //The prev_alloc field is 0, indicating that the previous block is free but the alloc field of the previous block header is not 0.*/
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(ptr));
    if(prev_alloc == 0){
        size_t alloc2 = GET_ALLOC(HDRP(PREV_BLKP(ptr)));
        if(alloc2 != 0){
            return 0;
        }
    }

    //The header of the block is before the end of the prologue, or the footer of the block is after the beginning of the epilogue.
    if(!((HDRP(ptr) - (prologue_header + 64)) >=0)){
        return 0;
    }
    if(!((epilogue_header - (FTRP(ptr) + 8)) >=0)){
        return 0;
    }

    //the pointer is not aligned to a 64 byte boundary
    if((unsigned long)ptr % 64 != 0){
        return 0;
    }
    return 1;
}

//check if the given block is the wilderness block
int is_wilderness(sf_block *block){
    size_t size = GET_SIZE(HDRP(NEXT_BLKP((char *)block + 16)));    //size of the next block
    size_t alloc= GET_ALLOC(HDRP(NEXT_BLKP((char *)block + 16)));       //alloc bit of the next block
    if(size && alloc){
        return 1;
    }
    else return 0;
}