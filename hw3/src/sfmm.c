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

unsigned long get_block_bize(unsigned long size);
void set_heap();
char *my_heap;
void *prologue_header;
void *epilogue_header;

void *sf_malloc(size_t size) {

    //Set up heap, prologue and epilogue for the first call
    if(my_heap == NULL){
        //NULL when no memory left or any error occured
        if((my_heap = (char *)sf_mem_grow()) == NULL){
            return NULL;
        }
        else{
            //set up heap and add unused block in free list
            set_heap();
        }
    }

    unsigned long blocksize;

    //verify the size
    if((int) size == 0){
        return NULL;
    }

    //check if the size is too big to allocate (due to limited MAX blocksize)
    if(size > (ULONG_MAX -8)){
        return NULL;
    }

    //calculate the blocksize
    blocksize = get_block_bize(size);

    //check if the required blocksize is greater than max value of unsigned long
    if (size >= blocksize){
        //blocksize will be less than the size if rounding up resulted in a roll over
        return NULL;
    }



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
unsigned long get_block_bize(unsigned long size){
    size = size + 8;
    /*size = ((size/64)*64) + 64;*/
    size = ((size+63) /64) * 64;
    return size;
}

//add prologue and epilogue in heap
void set_heap(){
    prologue_header = my_heap +56;  //get to the point
    //create the macros
    //set the value
    //get to footer
    //set the footer

    //get the last address from sf_mem_end
    //go back 8 bytes
    //set the header here


    //create free block out of the remaining memory
    //put the remaining block in the free list

}