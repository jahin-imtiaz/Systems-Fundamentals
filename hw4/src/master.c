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
void continue_and_terminate_workers();
int get_exit_abort_count();
int is_success();
void free_workers();

struct signals signal_queue;             //maintain a data structure for signals
struct worker worker_list_head;          //maintain a data structure for workers state
struct worker idle_worker_queue;     //maintain a data structure for idle workers

int null_problem_flag = 0;

void sigchld_handler(int sig) /* SIGTERM handler */
{

    int old_errno = errno;

    int status, pid;
    while((pid = waitpid(-1, &status, (WNOHANG|WUNTRACED)|WCONTINUED)) > 0){
        enqueue_signal(pid, status);
        debug("RECEIVED SIGCHLD FROM WORKER : %d",pid);
    }

    errno = old_errno;

}

int master(int workers) {
    // TO BE IMPLEMENTED
    sf_start();
    //install signal handlers
    if (signal(SIGCHLD, sigchld_handler) == SIG_ERR)
         return EXIT_FAILURE;

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
         return EXIT_FAILURE;

    int i, pid;

    //set the masks
    sigset_t mask, prev_mask/*, old*/;
    sigemptyset(&mask);
    /*sigemptyset(&old);*/
    sigaddset(&mask, SIGCHLD);

    int fd_list[workers][4];     //maintain a data structure for pipes

    initialize_fd_array(fd_list, workers);

    initialize_sig_queue();

    initialize_worker_list();

    initialize_idle_worker_queue();

    for(i = 0; i < workers; i++){      //create worker processes

        if((pid =fork()) == 0){         //child process

            dup2(fd_list[i][0] , 0);     //redirect stdin
            dup2(fd_list[i][3] , 1);     //redirect stdout

            close(fd_list[i][1]);       //close opposite side of pipe 1
            close(fd_list[i][2]);       //close opposite side of pipe 2

            char **argv = NULL;
            execv("bin/polya_worker", argv);

        }
        else{

            /*sigsuspend(&old);*/

            close(fd_list[i][0]);       //close opposite side of pipe 1
            close(fd_list[i][3]);       //close opposite side of pipe 2

            struct worker *nworker = malloc(sizeof(struct worker));
            nworker->pid = pid;                          //store the pid of the worker process
            nworker->worker_number = i;                  //store the worker number

            sf_change_state(pid, 0, WORKER_STARTED);
            nworker->current_state = WORKER_STARTED;     //change the current state

            (worker_list_head.prev)->next = nworker;   //add the worker to the list
            nworker->prev = worker_list_head.prev;
            nworker->next = &worker_list_head;
            worker_list_head.prev = nworker;
        }
    }


    while(1){   //break if get variant returns null and all workers are idle

        //check if any SIGCHLD signal is received in the queue and take action
        while(signal_queue.next != &signal_queue){     //if empty, signal_queue's next should wrap itself

            sigprocmask(SIG_BLOCK, &mask, &prev_mask); //block signal
            struct signals *tmp_sig = dequeue_signal();
            sigprocmask(SIG_SETMASK, &prev_mask, NULL);

            int pid2 = tmp_sig->pid;

            int status = tmp_sig->status;

            struct worker *tmp_worker = get_worker(pid2);    //get the worker related to the signal

            if(WIFEXITED(status)){      //child has exited

                if(WEXITSTATUS(status) == 0){   //exited normally

                    sf_change_state(tmp_worker->pid, tmp_worker->current_state, WORKER_EXITED);
                    tmp_worker->current_state = WORKER_EXITED;

                }
                else{   //or exited abnormally

                    sf_change_state(tmp_worker->pid, tmp_worker->current_state, WORKER_ABORTED);
                    tmp_worker->current_state = WORKER_ABORTED;

                }

            }
            else if(WIFSIGNALED(status)){       //aborted because of a signal

                sf_change_state(tmp_worker->pid, tmp_worker->current_state, WORKER_ABORTED);
                tmp_worker->current_state = WORKER_ABORTED;

            }
            else{       //SIGCHLD has been received

                if(WIFSTOPPED(status)){     //worker stopped

                    //if the workers current state is STARTED, then it should be IDLE now
                    if(tmp_worker->current_state == WORKER_STARTED){

                        sf_change_state(tmp_worker->pid, tmp_worker->current_state, change_state(tmp_worker->current_state));
                        tmp_worker->current_state = change_state(tmp_worker->current_state);

                        //add this worker to the idle list
                        enqueue_idle_worker(tmp_worker);

                    }
                    //if the workers current state is CONTINUED, then it should be STOPPED now (missed the signal for running)
                    else if(tmp_worker->current_state == WORKER_CONTINUED){

                        //change it from CONTINUED to RUNNING
                        sf_change_state(tmp_worker->pid, tmp_worker->current_state, change_state(tmp_worker->current_state));
                        tmp_worker->current_state = change_state(tmp_worker->current_state);

                        //change it from RUNNING to STOPPED
                        sf_change_state(tmp_worker->pid, tmp_worker->current_state, change_state(tmp_worker->current_state));
                        tmp_worker->current_state = change_state(tmp_worker->current_state);

                        //read result from the pipe
                        struct result *result_header = malloc(sizeof(struct result));

                        /*read(fd_list[tmp_worker->worker_number][2], result_header, sizeof(struct result));      //read the header*/
                        FILE *read_file = fdopen(fd_list[tmp_worker->worker_number][2], "r");
                        for(i = 0; i < sizeof(struct result); i++){

                            *((char *)result_header+i) = fgetc(read_file);

                        }

                        size_t result_size = result_header->size;              //get the total size of the result

                        result_header = realloc(result_header, result_size);   //realloc new size to fit the whole result

                        /*read(fd_list[tmp_worker->worker_number][2], ((char *)result_header)+sizeof(struct result), (result_size - sizeof(struct result)));//read the whole result*/
                        for(i = 0; i < (result_size - sizeof(struct result)); i++){

                            *((char *)result_header+(sizeof(struct result)+i)) = fgetc(read_file);

                        }

                        fclose(read_file);

                        sf_recv_result(tmp_worker->pid, result_header);

                        //check result
                        int rvalue = post_result(result_header, tmp_worker->current_problem);

                        //if correct, cancel all jobs
                        if(rvalue == 0){

                            //send SIGHUP to all other workers except the current one
                            struct worker *tmp = &worker_list_head;
                            while(tmp->next != &worker_list_head){

                                if((tmp->next)->worker_number != tmp_worker->worker_number ){   //if its not the current worker

                                    sf_cancel((tmp->next)->pid);

                                    kill((tmp->next)->pid, SIGHUP);     //send SIGHUP signal to cancel the current job
                                    /*sigsuspend(&old);*/

                                }

                                tmp = tmp->next;

                            }

                        }

                        //free the problem itself and set current_problem of the worker to be NULL because it is done with the current problem
                        free(tmp_worker->current_problem);
                        tmp_worker->current_problem = NULL;

                        //change state from STOPPED to IDLE
                        sf_change_state(tmp_worker->pid, tmp_worker->current_state, change_state(tmp_worker->current_state));
                        tmp_worker->current_state = change_state(tmp_worker->current_state);

                        //add this worker to the idle list
                        enqueue_idle_worker(tmp_worker);

                        free(result_header);

                    }
                    //if the workers current state is RUNNING, then it should be STOPPED now
                    //So, Read the result and post it and change the state of the worker to IDLE
                    else if(tmp_worker->current_state == WORKER_RUNNING){

                        //change it from RUNNING to STOPPED
                        sf_change_state(tmp_worker->pid, tmp_worker->current_state, change_state(tmp_worker->current_state));
                        tmp_worker->current_state = change_state(tmp_worker->current_state);

                        //read result from the pipe
                        struct result *result_header = malloc(sizeof(struct result));

                        /*read(fd_list[tmp_worker->worker_number][2], result_header, sizeof(struct result));      //read the header*/
                        FILE *read_file = fdopen(fd_list[tmp_worker->worker_number][2], "r");
                        for(i = 0; i < sizeof(struct result); i++){

                            *((char *)result_header+i) = fgetc(read_file);

                        }

                        size_t result_size = result_header->size;              //get the total size of the result

                        result_header = realloc(result_header, result_size);   //realloc new size to fit the whole result

                        /*read(fd_list[tmp_worker->worker_number][2], ((char *)result_header)+sizeof(struct result), (result_size - sizeof(struct result)));//read the whole result*/
                        for(i = 0; i < (result_size - sizeof(struct result)); i++){

                            *((char *)result_header+(sizeof(struct result)+i)) = fgetc(read_file);

                        }

                        /*fclose(read_file);*/

                        sf_recv_result(tmp_worker->pid, result_header);

                        //check result
                        int rvalue = post_result(result_header, tmp_worker->current_problem);

                        //if correct, cancel all jobs
                        if(rvalue == 0){

                            //send SIGHUP to all other workers except the current one
                            struct worker *tmp = &worker_list_head;
                            while(tmp->next != &worker_list_head){

                                if((tmp->next)->worker_number != tmp_worker->worker_number ){   //if its not the current worker

                                    sf_cancel((tmp->next)->pid);

                                    kill((tmp->next)->pid, SIGHUP);     //send SIGHUP signal to cancel the current job
                                    /*sigsuspend(&old);*/

                                }

                                tmp = tmp->next;

                            }

                        }

                        //set current_problem of the worker to be NULL because it is done with the current problem
                        tmp_worker->current_problem = NULL;

                        //change state from STOPPED to IDLE
                        sf_change_state(tmp_worker->pid, tmp_worker->current_state, change_state(tmp_worker->current_state));
                        tmp_worker->current_state = change_state(tmp_worker->current_state);

                        //add this worker to the idle list
                        enqueue_idle_worker(tmp_worker);

                        free(result_header);
                    }

                }
                else if(WIFCONTINUED(status)){  //worker continued

                    //if the workers current state is CONTINUED, then it should be RUNNING now
                    if(tmp_worker->current_state == WORKER_CONTINUED){

                        sf_change_state(tmp_worker->pid, tmp_worker->current_state, change_state(tmp_worker->current_state));
                        tmp_worker->current_state = change_state(tmp_worker->current_state);

                    }

                }

            }
            free(tmp_sig);

        }

        if(!null_problem_flag){
            //check if any worker is in idle mode or not
            //assign problem if idle and get variant is non NULL and also update the workers current_problem value
            if(idle_worker_queue.next_idle != &idle_worker_queue){
                //assign problems

                //get variant of problem
                struct problem *new_problem = get_problem_variant(workers, (idle_worker_queue.next_idle)->worker_number);

                if(new_problem == NULL){

                    null_problem_flag = 1;

                }
                else{

                    //assign workers current problem as this one
                    (idle_worker_queue.next_idle)->current_problem = new_problem;

                    //change state of the worker from IDLE to CONTINUED
                    sf_change_state((idle_worker_queue.next_idle)->pid, (idle_worker_queue.next_idle)->current_state, change_state((idle_worker_queue.next_idle)->current_state));
                    (idle_worker_queue.next_idle)->current_state = WORKER_CONTINUED;

                    //send SIGCONT signal
                    kill((idle_worker_queue.next_idle)->pid, SIGCONT);
                    /*sigsuspend(&old);*/

                    // write problem to the pipe
                    /*write(fd_list[(idle_worker_queue.next_idle)->worker_number][1], mallocd_problem, mallocd_problem->size);*/
                    sf_send_problem((idle_worker_queue.next_idle)->pid, new_problem);

                    FILE *write_file = fdopen(fd_list[(idle_worker_queue.next_idle)->worker_number][1], "w");
                    for(i = 0; i < new_problem->size; i++){

                        fputc(*((char *)new_problem+i), write_file);

                    }
                    fflush(write_file);
                    /*fclose(write_file);*/

                    //remove the worker from the idle list
                    dequeue_idle_worker();

                }

            }
        }
        else{
            //check if idle worker count is equal to the number of workers
            //if yes, break from the loop and terminate
            if(get_idle_count() == (workers - get_exit_abort_count())){

                break;

            }
        }

    }
    //terminate all the running workers
    //free all problems in the workers
    continue_and_terminate_workers();

    while(get_exit_abort_count() != workers){        //while atleast one worker hasn't exited or aborted

        while(signal_queue.next != &signal_queue){      //while there is atleast one unhandled signal

            sigprocmask(SIG_BLOCK, &mask, &prev_mask);
            struct signals *tmp_sig = dequeue_signal();
            sigprocmask(SIG_SETMASK, &prev_mask, NULL);

            int pid3 = tmp_sig->pid;

            int status2=tmp_sig->status;

            struct worker *tmp_worker2 = get_worker(pid3);    //get the worker related to the signal

            if(WIFEXITED(status2)){      //child has exited

                if(WEXITSTATUS(status2) == 0){   //exited normally

                    sf_change_state(tmp_worker2->pid, tmp_worker2->current_state, WORKER_EXITED);
                    tmp_worker2->current_state = WORKER_EXITED;

                }
                else{   //or exited abnormally

                    sf_change_state(tmp_worker2->pid, tmp_worker2->current_state, WORKER_ABORTED);
                    tmp_worker2->current_state = WORKER_ABORTED;

                }

            }
            else if(WIFSIGNALED(status2)){       //aborted because of a signal

                sf_change_state(tmp_worker2->pid, tmp_worker2->current_state, WORKER_ABORTED);
                tmp_worker2->current_state = WORKER_ABORTED;

            }
            else {      //else child has continued due to SIGCONT

                sf_change_state(tmp_worker2->pid, tmp_worker2->current_state, WORKER_RUNNING);
                tmp_worker2->current_state = WORKER_RUNNING;

            }

            free(tmp_sig);

        }
    }
    //pipes can fail. so check it.
    //return EXIT_SUCCESS if all workers have exited normally, EXIT_FAILURE otherwise
    if(is_success()){
        //free  all workers first
        free_workers();

        sf_end();
        return EXIT_SUCCESS;
    }
    else{
        //free  all workers first
        free_workers();

        sf_end();
        return EXIT_FAILURE;
    }

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

    worker_list_head.pid = -1;
    worker_list_head.worker_number = -1;
    worker_list_head.next = &worker_list_head;
    worker_list_head.prev = &worker_list_head;
}

void initialize_idle_worker_queue(){

    idle_worker_queue.pid = -1;
    idle_worker_queue.worker_number = -1;
    idle_worker_queue.next_idle = &idle_worker_queue;
    idle_worker_queue.prev_idle = &idle_worker_queue;
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

        if((tmp->next)->pid == pid){
            break;
        }

        tmp = tmp->next;
    }

    return tmp->next;
}

void initialize_sig_queue(){

    signal_queue.pid = -1;
    signal_queue.status = -1;
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

void continue_and_terminate_workers(){

    /*sigset_t old;
    sigemptyset(&old);*/

    struct worker *tmp = &worker_list_head;

    while(tmp->next != &worker_list_head){

        kill((tmp->next)->pid, SIGTERM);
        kill((tmp->next)->pid, SIGCONT);
        /*sigsuspend(&old);*/

        free((tmp->next)->current_problem);
        tmp = tmp->next;

    }

}

int get_exit_abort_count(){

    int count = 0;
    struct worker *tmp = &worker_list_head;
    while(tmp->next != &worker_list_head){

        if(((tmp->next)->current_state == WORKER_EXITED) || ((tmp->next)->current_state == WORKER_ABORTED)){
            count++;
        }

        tmp = tmp->next;
    }

    return count;
}

//returns 1 if all workers exited normally, 0 otherwise
int is_success(){

    struct worker *tmp = &worker_list_head;
    while(tmp->next != &worker_list_head){

        if((tmp->next)->current_state == WORKER_ABORTED){
            return 0;
        }

        tmp = tmp->next;
    }
    return 1;
}

void free_workers(){

    struct worker* tmp;

    while (worker_list_head.next != &worker_list_head)
    {
       tmp = worker_list_head.next;
       worker_list_head.next = (worker_list_head.next)->next;
       (worker_list_head.next)->prev = &worker_list_head;
       free(tmp);
    }

}