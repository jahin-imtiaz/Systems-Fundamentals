#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "debug.h"
#include "polya.h"

#include "myprototypes.h"

volatile sig_atomic_t sighup_flag = 0;
volatile sig_atomic_t sigterm_flag = 0;

void sighup_handler(int sig) /* SIGHUP handler */
{

    int old_errno = errno;
    sighup_flag = 1;    //this flag was received and the problem solver should cancel the problem
    errno = old_errno;
}

void sigterm_handler(int sig) /* SIGTERM handler */
{

    int old_errno = errno;
    sigterm_flag = 1;
    errno = old_errno;

}

/*
 * worker
 * (See polya.h for specification.)
 */
int worker(void) {
    // TO BE IMPLEMENTED
    int i;

    //install signal handlers
    if (signal(SIGHUP, sighup_handler) == SIG_ERR)
         return EXIT_FAILURE;

    if (signal(SIGTERM, sigterm_handler) == SIG_ERR)
         return EXIT_FAILURE;

    //go to the idle state
    kill(getpid(), SIGSTOP);

    while(!sigterm_flag){

        //read a problem
        struct problem *problem_header = malloc(sizeof(struct problem));

        /*read(0, problem_header, sizeof(struct problem));      //read the header*/
        for(i = 0; i < sizeof(struct problem); i++){

            *((char *)problem_header+i) = fgetc(stdin);

        }

        size_t prob_size = problem_header->size;              //get the total size of the problem

        problem_header = realloc(problem_header, prob_size);  //realloc new size to fit the whole problem

        /*read(0, ((char *)problem_header)+sizeof(struct problem), (prob_size - sizeof(struct problem)));//read the whole problem*/
        for(i = 0; i < (prob_size - sizeof(struct problem)); i++){

            *(((char *)problem_header)+(sizeof(struct problem)+i)) = fgetc(stdin);

        }

        struct result *prob_result = (solvers[problem_header->type].solve)(problem_header, &sighup_flag); //solve the problem


        if(prob_result != NULL){    //NULL if solver is cancelled or malloc error

            /*write(1, prob_result, prob_result->size);    //write the result*/
            for(i = 0; i < prob_result->size; i++){

                fputc(*((char *)prob_result+i), stdout);

            }
            fflush(stdout);

        }
        else{

            sighup_flag = 0;    //reset the sighup flag

            struct result tmp;  //write an empty result with failed flag enable
            tmp.size = sizeof(struct result);
            tmp.failed = 1;
            /*write(1,  &tmp, sizeof(struct result));*/
            for(i = 0; i < sizeof(struct result); i++){

                fputc(*((char *)&tmp+i), stdout);

            }
            fflush(stdout);

        }

        kill(getpid(), SIGSTOP);//stop and go back to idle state

        //free the variables
        free(problem_header);
        free(prob_result);

    }

    sigterm_flag = 0;

    return EXIT_SUCCESS;
}
