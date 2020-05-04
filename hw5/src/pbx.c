#include "pbx.h"

PBX *pbx_init();
void pbx_shutdown(PBX *pbx);
TU *pbx_register(PBX *pbx, int fd);
int pbx_unregister(PBX *pbx, TU *tu);
int tu_fileno(TU *tu);
int tu_extension(TU *tu);
int tu_pickup(TU *tu);
int tu_hangup(TU *tu);
int tu_dial(TU *tu, int ext);
int tu_chat(TU *tu, char *msg);