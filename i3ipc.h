#ifndef I3IPC_H
#define I3IPC_H
extern void* listen_to_i3(void*);
typedef struct {
  int num;
  int visible;
} Workspaces;
extern Workspaces workspaces[20];
#endif