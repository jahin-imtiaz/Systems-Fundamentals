
#ifndef MYPROTOTYPES_H
#define MYPROTOTYPES_H

#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

struct worker{
    int pid;
    int worker_number;
    int current_state;
    struct problem *current_problem;

    struct worker *next;    //pointer in the worker list
    struct worker *prev;    //pointer in the worker list

    struct worker *next_idle;    //pointer in the idle_worker list
    struct worker *prev_idle;    //pointer in the idle_worker list
};

struct signals
{
    int pid;
    int status;
    struct signals *next;
    struct signals *prev;
};

typedef void (*sighandler_t)(int);   //can now use sighandler_t SIGKILL

#endif