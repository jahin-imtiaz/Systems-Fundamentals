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
void free_list_init();
sf_block *find_fit(size_t size);
void split_block(sf_block *bp, size_t size);
char *my_heap;
char *prologue_header;
char *epilogue_header;

void *sf_malloc(size_t size) {
    sf_block *bp;
    //Set up heap, prologue and epilogue for the first call
    if(my_heap == NULL){
        //NULL when no memory left or any error occured
        if((my_heap = (char *)sf_mem_grow()) == NULL){
            return NULL;
        }
        else{
            //initialize the free list
            free_list_init();
            //set up heap and add unused block in free list
            set_heap();
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
void free_list_init(){
    int i;
    for(i = 0; i < NUM_FREE_LISTS; i++){
        sf_block sentinel;
        sentinel.body.links.next = &sentinel;
        sentinel.body.links.prev = &sentinel;
        sf_free_list_heads[i] =sentinel;
    }
}

//add the wilderness block in the free list
void add_wilderness_block(sf_block *block){
    (*block).body.links.next = sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next;

    (*block).body.links.prev = &sf_free_list_heads[NUM_FREE_LISTS-1];

    (*(*block).body.links.next).body.links.prev = block;

    sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next = block;
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
                return ptr->body.links.next;
            }
            ptr = ptr->body.links.next ;
        }
    }
    //no big enough block is found
    return NULL;
}

void split_block(sf_block *bp, size_t size){

}
//add a free block in the free list
void add_free_block(){

}