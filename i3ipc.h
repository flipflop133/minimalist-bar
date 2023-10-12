#ifndef I3IPC_H
#define I3IPC_H
extern void* listen_to_i3(void*);
typedef struct {
  unsigned char num;
  unsigned char visible;
  unsigned char urgent;
} Workspace;
typedef struct {
  Workspace * workspaces;
  unsigned short size;
} Workspaces;
extern Workspaces workspaces;
#endif