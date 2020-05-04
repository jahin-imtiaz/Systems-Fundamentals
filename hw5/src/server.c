#include "csapp.h"
#include "pbx.h"
#include "debug.h"
#include <string.h>
#include <server.h>

void execute_command(char *string,TU *clientTU);

void *pbx_client_service(void *arg){

    int connfd = *((int *)arg);
    Pthread_detach(pthread_self());
    Free(arg);

    TU *clientTU = pbx_register(pbx, connfd);
    FILE *file = fdopen(connfd, "r+");

    int base_size = 15;
    int cur_str_length = 0;
    char *string;

    string = malloc(15);

    int c;
    while((c=fgetc(file)) != EOF){

        string[cur_str_length++] = c;

        if(base_size == cur_str_length){

            base_size += 15;
            string = realloc(string, base_size);

        }
        //read a message. Each message ends with EOL
        if(strstr(string, EOL)){    //message read complete

            string[cur_str_length-2] = '\0';    //insert null character to indicate end of string

            execute_command(string, clientTU);

            base_size = 15;
            cur_str_length = 0;
            free(string);
            string = malloc(base_size);
        }

    }
    free(string);

    pbx_unregister(pbx, clientTU);

    Close(connfd);
    return NULL;
}

void execute_command(char *string, TU *clientTU){

    //get the first word in the string
    char *word = strtok(string , " ");

    if(strcmp(word, tu_command_names[TU_PICKUP_CMD]) == 0){

        //if there is any more token, then this message is invalid, otherwise valid
        if(strtok(NULL, " ") == NULL){
            tu_pickup(clientTU);
        }

    }
    else if(strcmp(word, tu_command_names[TU_HANGUP_CMD]) == 0){

        //if there is any more token, then this message is invalid
        if(strtok(NULL, " ") == NULL){
            tu_hangup(clientTU);
        }

    }
    else if(strcmp(word, tu_command_names[TU_DIAL_CMD]) == 0){

        char *scndWord;
        if((scndWord= strtok(NULL, " ")) != NULL){

            tu_dial(clientTU, atoi(scndWord));

        }

    }
    else if(strcmp(word, tu_command_names[TU_CHAT_CMD]) == 0){

        char *msg;
        if((msg = strtok(NULL, "\0")) != NULL){

            tu_chat(clientTU, msg);

        }

    }
}