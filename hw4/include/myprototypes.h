
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
    struct worker *next;
    struct worker *prev;
};

typedef void (*sighandler_t)(int);   //can now use sighandler_t SIGKILL

#endif