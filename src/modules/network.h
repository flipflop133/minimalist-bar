#ifndef NETWORK_H
#define NETWORK_H
#include "../defs.h"
void *wifi_update(void *arg);
#define NETWORK_BUFFER 30
enum interface {WIFI, ETHERNET};
#endif