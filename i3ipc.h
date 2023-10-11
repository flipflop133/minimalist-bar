#ifndef I3IPC_H
#define I3IPC_H
extern void* listen_to_i3(void*);
typedef struct {
  int num;
  int visible;
} Workspace;
typedef struct {
  Workspace * workspaces;
  unsigned short size;
} Workspaces;
extern Workspaces workspaces;
#endif