
#ifndef MYPROTOTYPES_H
#define MYPROTOTYPES_H

#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

/*A worker structure holds a workers current information */
struct worker{
    int pid;
    int worker_number;
    int current_state;
    struct problem *current_problem;

    FILE *read_file;
    FILE *write_file;

    struct worker *next;    //pointer in the worker list
    struct worker *prev;    //pointer in the worker list

    struct worker *next_idle;    //pointer in the idle_worker list
    struct worker *prev_idle;    //pointer in the idle_worker list
};

/*A signal structure holds information related to the signal received*/
struct signals
{
    int pid;
    int status;
    struct signals *next;
    struct signals *prev;
};
#endif