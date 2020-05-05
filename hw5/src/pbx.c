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
    struct tu *ringing_tu;
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

    if((pbx == NULL) || (tu == NULL)){
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

int pbx_unregister(PBX *pbx, TU *tu){

    //USE MUTEX
    if((pbx == NULL) || (tu == NULL) || (pbx->extension_array[tu->extention] == NULL)){
        return -1;
    }
    else{

        pbx->extension_array[tu->extention] = NULL;
        free(tu);
        return 0;

    }
    //USE MUTEX

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

        return 0;
    }

}

int tu_hangup(TU *tu){

    if(tu == NULL){
        return -1;
    }
    else{

        if(tu->current_state == TU_CONNECTED){

            tu->current_state = TU_ON_HOOK;

            (tu->connected_tu)->current_state = TU_DIAL_TONE;

            send_notification(TU_ON_HOOK, tu->fileno, 0);
            send_notification(TU_DIAL_TONE, (tu->connected_tu)->fileno, 0);

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

            (tu->ringing_tu)->ringing_tu = NULL;
            tu->ringing_tu = NULL;

        }
        else if(tu->current_state == TU_RINGING){

            tu->current_state = TU_ON_HOOK;

            (tu->ringing_tu)->current_state = TU_DIAL_TONE;

            send_notification(TU_ON_HOOK, tu->fileno, 0);
            send_notification(TU_DIAL_TONE, (tu->ringing_tu)->fileno, 0);

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

        return 0;
    }

}

int tu_dial(TU *tu, int ext);
int tu_chat(TU *tu, char *msg);

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