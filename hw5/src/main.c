#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "pbx.h"
#include "server.h"
#include "debug.h"
#include "csapp.h"

static void terminate(int status);

/*
 * "PBX" telephone exchange simulation.
 *
 * Usage: pbx <port>
 */

void sigchld_handler(int sig) /* SIGTERM handler */
{

    int old_errno = errno;
    terminate(EXIT_SUCCESS);
    errno = old_errno;

}

void sigpipe_handler(int sig) /* SIGTERM handler */
{

    int old_errno = errno;
    errno = old_errno;

}

int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    int option;
    char *port;
    while((option = getopt(argc, argv, "p:")) != EOF) {         //get the port # from the command line
        switch(option) {
            case 'p':
                port = optarg++;
                if(atoi(port) < 0) {
                fprintf(stderr, "-p (port number) requires a nonnegative argument\n");
                exit(EXIT_FAILURE);
                }
                break;
            default:
                fprintf(stderr, "Unknown option\n");
                exit(EXIT_FAILURE);
        }
    }

    // Perform required initialization of the PBX module.
    debug("Initializing PBX...");
    pbx = pbx_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function pbx_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.

    /*struct sigaction sig1;              //install SIGHUP handler
    sig1.sa_handler = sigchld_handler;
    sigaction(SIGHUP, &sig1, NULL);

    struct sigaction sig2;              //install SIGPIPE handler
    sig2.sa_handler = sigpipe_handler;
    sig2.sa_flags = 0;
    sigaction(SIGPIPE, &sig2, NULL);*/

    Signal(SIGHUP, sigchld_handler);
    Signal(SIGPIPE, sigpipe_handler);

    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    listenfd = Open_listenfd(port);     //open socket, bind and listen to the listenfd

    while (1) {
        clientlen=sizeof(struct sockaddr_storage);
        connfdp = malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);    //accept connections
        Pthread_create(&tid, NULL, pbx_client_service, connfdp);        //create threads for each connection
    }

    fprintf(stderr, "You have to finish implementing main() "
	    "before the PBX server will function.\n");

    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int status) {
    debug("Shutting down PBX...");
    pbx_shutdown(pbx);
    debug("PBX server terminating");
    exit(status);
}
