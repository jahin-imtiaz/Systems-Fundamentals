#include "csapp.h"
#include "pbx.h"
#include "debug.h"
#include <string.h>
#include <server.h>

void execute_command(char *string,TU *clientTU);

void *pbx_client_service(void *arg){

    int connfd = *((int *)arg);                 //get the integer fd value for the underlying connection
    Pthread_detach(pthread_self());             //detach the thread
    Free(arg);                                  //free the malloced fd pointer

    TU *clientTU = pbx_register(pbx, connfd);   //register this client as a TU
    FILE *file = fdopen(connfd, "r+");

    int base_size = 15;                         //base size for client message buffer
    int cur_str_length = 0;                     //current length of the message line
    char *string;                               //buffer

    string = calloc(15, sizeof(char));                        //malloc storage for the buffer

    int c;
    while((c=fgetc(file)) != EOF){              //while the connection is not closed
        string[cur_str_length++] = c;           //store the character read in the buffer

        if(base_size == cur_str_length){        //check if our current buffer is full or not, realloc storage if full

            base_size += 15;
            string = realloc(string, base_size);

        }
        //read a message. Each message ends with EOL
        if(strstr(string, EOL)){    //message read complete, if EOL found the buffer

            string[cur_str_length-2] = '\0';    //insert null character to indicate end of string
            execute_command(string, clientTU);  //execute command from the message

            base_size = 15;                     //update the buffer informations here
            cur_str_length = 0;
            free(string);
            string = calloc(base_size, sizeof(char));
        }

    }
    free(string);                               //free the buffer
    pbx_unregister(pbx, clientTU);              //unregister the TU
    Close(connfd);                              //close the connection fd
    return NULL;
}

void execute_command(char *string, TU *clientTU){

    char *saveptr = string;
    char *word = strtok_r(string , " ", &saveptr);
    if(strcmp(word, tu_command_names[TU_PICKUP_CMD]) == 0){     //if pickup command
        //if there is any more token, then this message is invalid, otherwise valid
        if(strtok_r(NULL, " ", &saveptr) == NULL){
            tu_pickup(clientTU);
        }

    }
    else if(strcmp(word, tu_command_names[TU_HANGUP_CMD]) == 0){//if hangup command
        //if there is any more token, then this message is invalid
        if(strtok_r(NULL, " ", &saveptr) == NULL){
            tu_hangup(clientTU);
        }

    }
    else if(strcmp(word, tu_command_names[TU_DIAL_CMD]) == 0){  //if dial # command
        char *scndWord;
        if((scndWord= strtok_r(NULL, " ", &saveptr)) != NULL){

            tu_dial(clientTU, atoi(scndWord));

        }

    }
    else if(strcmp(word, tu_command_names[TU_CHAT_CMD]) == 0){  //if chat ... command
        char *msg;
        if((msg = strtok_r(NULL, "\0", &saveptr)) != NULL){
            tu_chat(clientTU, msg);

        }
        else{
            msg = "";
            tu_chat(clientTU, msg);
        }

    }
}