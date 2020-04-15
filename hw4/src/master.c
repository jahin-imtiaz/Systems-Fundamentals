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
void initialize_worker_list();
void initialize_idle_worker_queue();
void initialize_sig_queue();

struct worker *get_worker(int pid);
int get_idle_count();
int change_state(int current_state);
void enqueue_signal(int pid, int status);
struct signals *dequeue_signal();
void enqueue_idle_worker(struct worker *idle_workerp);
struct worker *dequeue_idle_worker();
void terminate_workers();

struct signals signal_queue;             //maintain a data structure for signals
struct worker worker_list_head;          //maintain a data structure for workers state
struct worker idle_worker_queue;     //maintain a data structure for idle workers

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
    int var = 0;

    int fd_list[workers][4];     //maintain a data structure for pipes

    initialize_fd_array(fd_list, workers);

    initialize_sig_queue();

    initialize_worker_list();

    initialize_idle_worker_queue();

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


    while(1){   //break if get variant returns null and all workers are idle

        //check if any SIGCHLD signal is received in the queue and take action
        while(signal_queue.next != &signal_queue){     //if empty, signal_queue's next should wrap itself

            struct signals *tmp_sig = dequeue_signal();

            int pid = tmp_sig->pid;

            struct worker *tmp_worker = get_worker(pid);    //get the worker related to the signal

            //if the workers current state is INIT, then it should be IDLE now
            if(tmp_worker->current_state == 0){

                tmp_worker->current_state = change_state(tmp_worker->current_state);

                //add this worker to the idle list


            }
            //if the workers current state is CONTINUED, then it should be RUNNING now
            else if(tmp_worker->current_state == WORKER_CONTINUED){

                tmp_worker->current_state = change_state(tmp_worker->current_state);

            }
            //if the workers current state is RUNNING, then it should be STOPPED now
            //So, Read the result and post it and change the state of the worker to IDLE
            else if(tmp_worker->current_state == WORKER_RUNNING){

                tmp_worker->current_state = change_state(tmp_worker->current_state);

                //read result from the pipe
                struct result *result_header = malloc(sizeof(struct result));

                read(fd_list[tmp_worker->worker_number][2], result_header, sizeof(struct result));      //read the header

                size_t result_size = result_header->size;              //get the total size of the result

                result_header = realloc(result_header, result_size);   //realloc new size to fit the whole result

                read(fd_list[tmp_worker->worker_number][2], ((char *)result_header)+sizeof(struct result), (result_size - sizeof(struct result)));//read the whole result

                //check result
                int rvalue = post_result(result_header, tmp_worker->current_problem);

                //if correct, cancel all jobs
                if(rvalue == 0){

                    //send SIGHUP to all other workers except the current one
                    struct worker *tmp = &worker_list_head;
                    while(tmp->next != &worker_list_head){

                        if((tmp->next)->worker_number != tmp_worker->worker_number ){   //if its not the current worker

                            kill((tmp->next)->pid, SIGHUP);     //send SIGHUP signal to cancel the current job

                        }

                    }

                    //reset var to zero for the next problem
                    var = 0;

                }

                //free the problem itself and set current_problem of the worker to be NULL because it is done with the current problem
                free(tmp_worker->current_problem);
                tmp_worker->current_problem = NULL;

                //change state from STOPPED to IDLE
                tmp_worker->current_state = change_state(tmp_worker->current_state);

                free(result_header);

            }

            free(tmp_sig);

        }

        //check if any worker is in idle mode or not
        //assign problem if idle and get variant is non NULL and also update the workers current_problem value
        if(idle_worker_queue.next_idle != &idle_worker_queue){
            //assign problems

            //get variant of problem
            struct problem *new_problem = get_problem_variant(workers, var++);

            if(new_problem == NULL){

                //check if idle worker count is equal to the number of workers
                //if yes, break from the loop and terminate
                if(get_idle_count() == workers){

                    break;

                }

            }
            else{

                //malloc the problem so we have access to it
                struct problem *mallocd_problem = malloc(new_problem->size);

                memcpy(mallocd_problem, new_problem, new_problem->size);

                //free the actual pointer returned by get variant
                free(new_problem);

                //assign workers current problem as this one
                (idle_worker_queue.next_idle)->current_problem = mallocd_problem;

                // write problem to the pipe
                write(fd_list[(idle_worker_queue.next_idle)->worker_number][1], mallocd_problem, mallocd_problem->size);

                //send SIGCONT signal
                kill((idle_worker_queue.next_idle)->pid, SIGCONT);

                //chage the workers state
                (idle_worker_queue.next_idle)->current_state = change_state((idle_worker_queue.next_idle)->current_state);

                //remove the worker from the idle list
                dequeue_idle_worker();

            }

        }
    }
    //terminate all the running workers
    //free all problems in the workers
    terminate_workers();

    //find out if some workers exited abnormally

    //manage workers that exited and aborted appropriately

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

void initialize_worker_list(){

    worker_list_head.next = &worker_list_head;
    worker_list_head.prev = &worker_list_head;
}

void initialize_idle_worker_queue(){

    worker_list_head.next_idle = &worker_list_head;
    worker_list_head.prev_idle = &worker_list_head;
}

//adds a idle worker in the queue
void enqueue_idle_worker(struct worker *idle_workerp){

    (idle_worker_queue.prev_idle)-> next_idle = idle_workerp;

    idle_workerp->prev_idle = idle_worker_queue.prev_idle;

    idle_workerp->next_idle = &idle_worker_queue;

    idle_worker_queue.prev_idle = idle_workerp;

}

//removes a idle worker from the queue and returns it
struct worker *dequeue_idle_worker(){

    struct worker * tmp = idle_worker_queue.next_idle;

    ((idle_worker_queue.next_idle)->next_idle)->prev_idle = &idle_worker_queue;

    idle_worker_queue.next_idle = (idle_worker_queue.next_idle)->next_idle;

    return tmp;
}

//given a state, it changes to the next state
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

struct worker *get_worker(int pid){

    struct worker *tmp = &worker_list_head;
    while(tmp->next != &worker_list_head){

        if((*(tmp->next)).pid == pid){
            return tmp->next;
        }

        tmp = tmp->next;
    }

    return NULL;
}

void initialize_sig_queue(){

    signal_queue.next = &signal_queue;
    signal_queue.prev = &signal_queue;

}

//adds a signals structure in the queue
void enqueue_signal(int pid, int status){

    struct signals * nsignals = (struct signals*) malloc(sizeof(struct signals));

    nsignals->pid  = pid;

    nsignals->status = status;

    (signal_queue.prev)-> next = nsignals;

    nsignals->prev = signal_queue.prev;

    nsignals->next = &signal_queue;

    signal_queue.prev = nsignals;

}

//removes a signals structure from the queue and returns it
struct signals *dequeue_signal(){

    struct signals * tmp = signal_queue.next;

    ((signal_queue.next)->next)->prev = &signal_queue;

    signal_queue.next = ((signal_queue.next)->next);

    return tmp;
}

int get_idle_count(){

    int count = 0;
    struct worker *tmp = &idle_worker_queue;
    while(tmp->next_idle != &idle_worker_queue){

        count++;

        tmp = tmp->next_idle;
    }

    return count;
}

void terminate_workers(){

    struct worker *tmp = &worker_list_head;
    while(tmp->next != &worker_list_head){

        kill((tmp->next)->pid, SIGTERM);
        free((tmp->next)->current_problem);
        tmp = tmp->next;

    }

}