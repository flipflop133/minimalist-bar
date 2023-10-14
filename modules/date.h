#ifndef DATE_H
#define DATE_H
#include "../defs.h"
#define DATE_BUFFER 13
struct Module;
extern void *date_update(void *, struct Module *module);
#endif