#include "pbx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void send_notification(int state, int fd, int connected_tu);

struct pbx{

    TU *extension_array[PBX_MAX_EXTENSIONS];

};

struct tu{

    int current_state;
    int extention;
    int fileno;
    struct tu *connected_tu;

};


PBX *pbx_init(){

    PBX *pb = malloc(sizeof(PBX));

    return pb;
}

void pbx_shutdown(PBX *pbx){
    //TODO
}

TU *pbx_register(PBX *pbx, int fd){

    TU *tu = malloc(sizeof(TU));

    if(tu == NULL){
        return NULL;
    }
    else{
        tu->current_state = TU_ON_HOOK;
        tu->extention = fd;
        tu->fileno = fd;

        send_notification(TU_ON_HOOK, fd, 0);

        //USE MUTEX
        pbx->extension_array[fd] = tu;  //add this TU in the pbx array
        //USER MUTEX

        return tu;
    }

}

int pbx_unregister(PBX *pbx, TU *tu);
int tu_fileno(TU *tu);
int tu_extension(TU *tu);
int tu_pickup(TU *tu);
int tu_hangup(TU *tu);
int tu_dial(TU *tu, int ext);
int tu_chat(TU *tu, char *msg);

void send_notification(int state, int fd, int connected_tu){

    char *buf;
    switch(state){

        case TU_ON_HOOK:
            buf = malloc(30);
            sprintf(buf, "%s %d", tu_state_names[state], state);
            write(fd, buf, strlen(buf));
            free(buf);
            break;

        case TU_RINGING:
            write(fd, tu_state_names[state], strlen(tu_state_names[state]));
            break;
        case TU_DIAL_TONE:
            write(fd, tu_state_names[state], strlen(tu_state_names[state]));
            break;
        case TU_RING_BACK:
            write(fd, tu_state_names[state], strlen(tu_state_names[state]));
            break;
        case TU_BUSY_SIGNAL:
            write(fd, tu_state_names[state], strlen(tu_state_names[state]));
            break;

        case TU_CONNECTED:
            buf = malloc(30);
            sprintf(buf, "%s %d", tu_state_names[state], connected_tu);
            write(fd, buf, strlen(buf));
            free(buf);
            break;

        case TU_ERROR:
            write(fd, tu_state_names[state], strlen(tu_state_names[state]));
            break;

    }
    /*write(fd, string, strlen(string));*/

}