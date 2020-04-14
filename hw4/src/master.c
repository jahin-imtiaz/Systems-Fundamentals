#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "debug.h"
#include "polya.h"

/*
 * master
 * (See polya.h for specification.)
 */
int master(int workers) {
    // TO BE IMPLEMENTED
    //maintain a data structure for workers state
    //create child processes
    //for each processes create two pipes and maintain a data structure
    //inside each child, redirect standard input and output (see coordination protocol for details)
    //execute the childs

    //enter main loop
    //repeatedly assign problems
    //receive results
    //post results
    return EXIT_FAILURE;
}
