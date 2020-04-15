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
void enqueue_signal(int pid, int status);

struct signals signal_queue;

void sigchld_handler(int sig) /* SIGTERM handler */
{

    int old_errno = errno;

    int status;
    int pid = waitpid(-1, &status, (WNOHANG|WUNTRACED)|WCONTINUED);
    enqueue_signal(pid, status);        //?????? use sigsuspend here

    errno = old_errno;

}

int master(int workers) {
    // TO BE IMPLEMENTED
    //install signal handlers
    if (signal(SIGCHLD, sigchld_handler) == SIG_ERR)
         return EXIT_FAILURE;

    int i, pid;

    int fd_list[workers][4];     //maintain a data structure for pipes

    initialize_fd_array(fd_list, workers);

    struct worker worker_list_head;      //maintain a data structure for workers state

    initialize_worker_list(worker_list_head);

    for(i = 0; i < workers; i++){      //create worker processes

        if((pid =fork()) == 0){         //child process

            dup2(fd_list[i][0] , 0);     //redirect stdin
            dup2(fd_list[i][3] , 1);     //redirect stdout

            //???????manage opening and closing of files
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


    while(1){

        //check if any signal is received in the queue and take action

        //check if any worker is in idle mode or not
        //assign problem if idle and get variant is non NULL

        //break if get variant returns null and all workers are idle
    }

    //loop through the worker list inifinitly

    //if empty, continue doing other things
    //repeatedly assign problems
    //receive results
    //post results
    //free the queues //free each time you use dequeue

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

int change_state(int current_state){
    switch(current_state){

        case WORKER_STARTED:
            return WORKER_IDLE;
            break;

        case WORKER_IDLE:
            return WORKER_CONTINUED;
            break;

        case WORKER_CONTINUED:
            return WORKER_RUNNING;
            break;

        case WORKER_RUNNING:
            return WORKER_STOPPED;
            break;

        case WORKER_STOPPED:
            return WORKER_IDLE;
            break;
        case WORKER_EXITED:
            return current_state;
            break;
        case WORKER_ABORTED:
            return current_state;
            break;
    }

    return 0;
}

void enqueue_signal(int pid, int status){

    struct signals * nsignals = (struct signals*) malloc(sizeof(struct signals));

    nsignals->pid  = pid;

    nsignals->status = status;

    (signal_queue.prev)-> next = nsignals;

    nsignals->prev = (signal_queue.prev)->next;

    nsignals->next = &signal_queue;

    signal_queue.prev = nsignals;

}

struct signals dequeue_signal(){

    struct signals * tmp = signal_queue.next;

    (signal_queue.next)->prev = &signal_queue;

    signal_queue.next = (signal_queue.next)->prev;

    return *tmp;
}

