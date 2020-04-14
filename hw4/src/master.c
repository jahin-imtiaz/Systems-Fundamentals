#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "debug.h"
#include "polya.h"

#include "myprototypes.h"

/*
 * master
 * (See polya.h for specification.)
 */
void initialize_fd_array(int arr[][4], int workers);
void initialize_worker_list(struct worker head);

int master(int workers) {
    // TO BE IMPLEMENTED
    int i, pid;

    int fd_list[workers][4];     //maintain a data structure for pipes

    initialize_fd_array(fd_list, workers);

    struct worker worker_list_head;      //maintain a data structure for workers state

    initialize_worker_list(worker_list_head);

    for(i = 0; i < workers; i++){      //create worker processes

        if((pid =fork()) == 0){         //child process

            dup2(fd_list[i][0] , 0);     //redirect stdin
            dup2(fd_list[i][3] , 1);     //redirect stdout

            char **argv = NULL;          //??????????????????????????????????
            execv("bin/polya_worker", argv);

        }
        else{
            struct worker nworker;
            nworker.pid = pid;
            nworker.worker_number = i;

            (worker_list_head.prev)->next = &nworker;   //add the worker to the list
            nworker.prev = worker_list_head.prev;
            nworker.next = &worker_list_head;
            worker_list_head.prev = &nworker;
        }
    }

    //enter main loop
    //repeatedly assign problems
    //receive results
    //post results
    return EXIT_FAILURE;
}

void initialize_fd_array(int arr[][4], int workers){
    int i;
    int fd1[2];
    int fd2[2];
    for(i = 0; i < workers; i++){

        pipe(fd1);
        pipe(fd2);

        arr[i][0] = fd1[0];
        arr[i][1] = fd1[1];
        arr[i][2] = fd2[0];
        arr[i][3] = fd2[1];
    }
}

void initialize_worker_list(struct worker head){

    head.next = &head;
    head.prev = &head;
}