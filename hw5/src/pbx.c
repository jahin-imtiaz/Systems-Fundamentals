#include "pbx.h"
#include "csapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <semaphore.h>

void send_notification(int state, int fd, int connected_tu);
int valid_extension(int ext);

int extensions_cnt;             //used to count current number of registered extensions
sem_t extensions_cnt_mutex;

int all_cancelled_flag;         //used to indicate all extensions are unregistered or not
sem_t all_cancelled_flag_mutex;

sem_t tu_mutex_array[PBX_MAX_EXTENSIONS]; //Used to lock each TU's individually

sem_t pbx_mutex;

struct pbx{

    TU *extension_array[PBX_MAX_EXTENSIONS];

};

struct tu{

    int current_state;
    int extention;
    int fileno;
    struct tu *ringing_tu;
    struct tu *connected_tu;

};


PBX *pbx_init(){

    PBX *pb = malloc(sizeof(PBX));

    //initialize the mutexes
    sem_init(&extensions_cnt_mutex, 0, 1);
    sem_init(&all_cancelled_flag_mutex, 0 , 1);

    sem_init(&pbx_mutex, 0, 1);

    int j;
    for(j = 0; j < PBX_MAX_EXTENSIONS; j++){
        pb->extension_array[j] = NULL;
        sem_init(&tu_mutex_array[j], 0, 1);
    }

    return pb;
}

void pbx_shutdown(PBX *pbx){
    //TODO

    P(&pbx_mutex);
    int i;
    for(i = 0; i < PBX_MAX_EXTENSIONS; i++){
        if(pbx->extension_array[i]  != NULL){
            shutdown((pbx->extension_array[i])->fileno, SHUT_RDWR);
        }
    }
    V(&pbx_mutex);

    //wait for all server threads to terminate
    P(&all_cancelled_flag_mutex);//wait until you have access to the all_cancelled_flag
    free(pbx);      //free the pbx object itself
    V(&all_cancelled_flag_mutex);

}

TU *pbx_register(PBX *pbx, int fd){
    P(&pbx_mutex);
    TU *tu = malloc(sizeof(TU));

    if((pbx == NULL) || (tu == NULL)){

        if(tu != NULL){        //only pbx was null
            free(tu);
        }
        V(&pbx_mutex);
        return NULL;
    }
    else{
        tu->current_state = TU_ON_HOOK;
        tu->extention = fd;
        tu->fileno = fd;
        tu->ringing_tu = NULL;
        tu->connected_tu = NULL;

        send_notification(TU_ON_HOOK, fd, 0);

        //USE MUTEX
        pbx->extension_array[fd] = tu;  //add this TU in the pbx array
        //USER MUTEX

        P(&extensions_cnt_mutex);
        extensions_cnt++;
        if(extensions_cnt == 1){        //first registered TU
            //lock the mutex that will only be unlocked when last TU deregisters
            P(&all_cancelled_flag_mutex);
        }
        V(&extensions_cnt_mutex);
        V(&pbx_mutex);
        return tu;
    }

}

int pbx_unregister(PBX *pbx, TU *tu){
    P(&pbx_mutex);
    if((pbx == NULL) || (tu == NULL) || (pbx->extension_array[tu->extention] == NULL)){
        V(&pbx_mutex);
        return -1;
    }
    else{

        pbx->extension_array[tu->extention] = NULL;
        free(tu);

        P(&extensions_cnt_mutex);
        extensions_cnt--;
        if(extensions_cnt == 0){        //last de-registered TU
            //release the lock of the mutex that will now indicate that All TU is deregistered
            //if you have access to this lock
            V(&all_cancelled_flag_mutex);
        }
        V(&extensions_cnt_mutex);
        V(&pbx_mutex);
        return 0;

    }

}

int tu_fileno(TU *tu){


    if(tu != NULL){
        return tu->fileno;
    }
    else{
        return -1;
    }
}

int tu_extension(TU *tu){

    if(tu != NULL){
        return tu->extention;
    }
    else{
        return -1;
    }
}

int tu_pickup(TU *tu){
    if(tu == NULL){
        return -1;
    }
    else{
        //acquire lock either for one or two TUs depending on the state
        if(tu->ringing_tu == NULL){     //acquire one lock
            P(&tu_mutex_array[tu->extention]);
        }
        else{   //acquire both locks in order
            if(tu->extention < (tu->ringing_tu)->extention){
                P(&tu_mutex_array[tu->extention]);
                P(&tu_mutex_array[(tu->ringing_tu)->extention]);
            }
            else{
                P(&tu_mutex_array[(tu->ringing_tu)->extention]);
                P(&tu_mutex_array[tu->extention]);

            }
        }

        if(tu->current_state == TU_ON_HOOK){
            tu->current_state = TU_DIAL_TONE;
            send_notification(TU_DIAL_TONE, tu->fileno, 0);

        }
        else if(tu->current_state == TU_RINGING){
            tu->current_state = TU_CONNECTED;
            tu->connected_tu = tu->ringing_tu;
            send_notification(TU_CONNECTED, tu->fileno, (tu->ringing_tu)->extention);

            (tu->ringing_tu)->current_state = TU_CONNECTED;
            (tu->ringing_tu)->connected_tu = tu;
            send_notification(TU_CONNECTED, (tu->ringing_tu)->fileno, tu->extention);
        }
        else{
            send_notification(tu->current_state, tu->fileno, 0);
        }

        //unlock either for one or two TUs depending on the state
        if(tu->ringing_tu == NULL){     //give away one lock
            V(&tu_mutex_array[tu->extention]);
        }
        else{   //giva away both locks in order

            V(&tu_mutex_array[(tu->ringing_tu)->extention]);
            V(&tu_mutex_array[tu->extention]);
        }
        return 0;
    }

}

int tu_hangup(TU *tu){
    int mutex_flag=0;
    int scndfd;

    if(tu == NULL){
        return -1;
    }
    else{
        //acquire lock either for one or two TUs depending on the state
        if(tu->ringing_tu == NULL){     //acquire one lock
            P(&tu_mutex_array[tu->extention]);
            mutex_flag = 1;

        }
        else{   //acquire both locks in order
            mutex_flag = 2;
            if(tu->extention < (tu->ringing_tu)->extention){
                P(&tu_mutex_array[tu->extention]);
                P(&tu_mutex_array[(tu->ringing_tu)->extention]);

            }
            else{
                P(&tu_mutex_array[(tu->ringing_tu)->extention]);

            }
        }
        if(tu->current_state == TU_CONNECTED){
            tu->current_state = TU_ON_HOOK;

            (tu->connected_tu)->current_state = TU_DIAL_TONE;

            send_notification(TU_ON_HOOK, tu->fileno, 0);
            send_notification(TU_DIAL_TONE, (tu->connected_tu)->fileno, 0);

            scndfd = (tu->connected_tu)->extention;
            (tu->connected_tu)->ringing_tu = NULL;
            (tu->connected_tu)->connected_tu = NULL;


            tu->ringing_tu = NULL;
            tu->connected_tu = NULL;

        }
        else if(tu->current_state == TU_RING_BACK){

            tu->current_state = TU_ON_HOOK;

            (tu->ringing_tu)->current_state = TU_ON_HOOK;

            send_notification(TU_ON_HOOK, tu->fileno, 0);
            send_notification(TU_ON_HOOK, (tu->ringing_tu)->fileno, 0);

            scndfd = (tu->ringing_tu)->extention;
            (tu->ringing_tu)->ringing_tu = NULL;
            tu->ringing_tu = NULL;

        }
        else if(tu->current_state == TU_RINGING){

            tu->current_state = TU_ON_HOOK;

            (tu->ringing_tu)->current_state = TU_DIAL_TONE;

            send_notification(TU_ON_HOOK, tu->fileno, 0);
            send_notification(TU_DIAL_TONE, (tu->ringing_tu)->fileno, 0);

            scndfd = (tu->ringing_tu)->extention;
            (tu->ringing_tu)->ringing_tu = NULL;
            tu->ringing_tu = NULL;

        }
        else if(tu->current_state == TU_DIAL_TONE || tu->current_state == TU_BUSY_SIGNAL || tu->current_state == TU_ERROR){
            tu->current_state = TU_ON_HOOK;
            send_notification(TU_ON_HOOK, tu->fileno, 0);
        }
        else{
            send_notification(tu->current_state, tu->fileno, 0);
        }

        //unlock either for one or two TUs depending on the state
        if(mutex_flag == 1){     //give away one lock
            V(&tu_mutex_array[tu->extention]);
        }
        else if(mutex_flag == 2){   //giva away both locks in order

            V(&tu_mutex_array[scndfd]);
            V(&tu_mutex_array[tu->extention]);
        }
        return 0;
    }

}

int tu_dial(TU *tu, int ext){
    if(valid_extension(ext)){
        P(&pbx_mutex);
        if((pbx->extension_array[ext]) != NULL){
            //lock both TU's in order
            if(tu->extention == (pbx->extension_array[ext])->extention){   //hold one lock if both TUs refers to the same tu
                P(&tu_mutex_array[tu->extention]);
            }
            else if(tu->extention < (pbx->extension_array[ext])->extention){
                P(&tu_mutex_array[tu->extention]);
                P(&tu_mutex_array[(pbx->extension_array[ext])->extention]);
            }
            else{
                P(&tu_mutex_array[(pbx->extension_array[ext])->extention]);
                P(&tu_mutex_array[tu->extention]);
            }
            if(tu->current_state == TU_DIAL_TONE){
                if((pbx->extension_array[ext])->current_state == TU_ON_HOOK){

                    tu->current_state = TU_RING_BACK;
                    tu->ringing_tu = (pbx->extension_array[ext]);

                    (pbx->extension_array[ext])->current_state = TU_RINGING;
                    (pbx->extension_array[ext])->ringing_tu = tu;

                    send_notification(TU_RING_BACK, tu->fileno, 0);
                    send_notification(TU_RINGING, (pbx->extension_array[ext])->fileno, 0);

                }
                else{
                    tu->current_state = TU_BUSY_SIGNAL;
                    send_notification(TU_BUSY_SIGNAL, tu->fileno, 0);

                }

            }
            else{
                send_notification(tu->current_state, tu->fileno, 0);

            }
            //Unlock both TU's
            if(tu->extention == (pbx->extension_array[ext])->extention){   //unlock one if both TUs refers to the same tu
                V(&tu_mutex_array[tu->extention]);

            }
            else{
                V(&tu_mutex_array[tu->extention]);
                V(&tu_mutex_array[(pbx->extension_array[ext])->extention]);
            }

        }
        else{
            P(&tu_mutex_array[tu->extention]);

            if(tu->current_state != TU_DIAL_TONE){   //current state is anything other than TU_DIAL_TONE,
                send_notification(tu->current_state, tu->fileno, 0);
            }
            else{
                tu->current_state = TU_ERROR;
                send_notification(TU_ERROR, tu->fileno, 0);
            }

            V(&tu_mutex_array[tu->extention]);

        }

        V(&pbx_mutex);
        return 0;

    }
    else return -1;

}
int tu_chat(TU *tu, char *msg){

    P(&tu_mutex_array[tu->extention]);

    if(tu->current_state == TU_CONNECTED){

        //send the specified msg to the peer tu
        write((tu->connected_tu)->fileno, "CHAT ", strlen("CHAT "));
        write((tu->connected_tu)->fileno, msg, strlen(msg));
        write((tu->connected_tu)->fileno, EOL, strlen(EOL));

        //notify the sender of its current state
        send_notification(TU_CONNECTED, tu->fileno, (tu->connected_tu)->extention);

        V(&tu_mutex_array[tu->extention]);
        return 0;
    }
    else{
        //notify the sender of its current state
        send_notification(tu->current_state, tu->fileno, 0);
        V(&tu_mutex_array[tu->extention]);
        return -1;
    }

}

/*
*Sends a notification of the current state "state" to the TU indicated by its fd.
*if state is TU_CONNECTED, then "connected_tu" is the extension of the other TU.
*/
void send_notification(int state, int fd, int connected_tu){

    //TAKE CARE OF WHICH TO USE: DPRINTF() OR SPRINTF()
    char *buf;
    switch(state){

        case TU_ON_HOOK:
            buf = malloc(30);
            sprintf(buf, "%s %d%s", tu_state_names[state], fd, EOL);
            write(fd, buf, strlen(buf));
            free(buf);
            break;

        case TU_RINGING:
            write(fd, tu_state_names[state], strlen(tu_state_names[state]));
            write(fd, EOL, strlen(EOL));
            break;
        case TU_DIAL_TONE:
            write(fd, tu_state_names[state], strlen(tu_state_names[state]));
            write(fd, EOL, strlen(EOL));
            break;
        case TU_RING_BACK:
            write(fd, tu_state_names[state], strlen(tu_state_names[state]));
            write(fd, EOL, strlen(EOL));
            break;
        case TU_BUSY_SIGNAL:
            write(fd, tu_state_names[state], strlen(tu_state_names[state]));
            write(fd, EOL, strlen(EOL));
            break;

        case TU_CONNECTED:
            buf = malloc(30);
            sprintf(buf, "%s %d%s", tu_state_names[state], connected_tu, EOL);
            write(fd, buf, strlen(buf));
            free(buf);
            break;

        case TU_ERROR:
            write(fd, tu_state_names[state], strlen(tu_state_names[state]));
            write(fd, EOL, strlen(EOL));
            break;

    }

}

int valid_extension(int ext){

    if(ext < 0 || ext > PBX_MAX_EXTENSIONS){
        return 0;
    }
    else return 1;
}