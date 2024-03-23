#ifndef I3IPC_H
#define I3IPC_H
extern void *listen_to_i3(void *);
typedef struct
{
  unsigned char num;
  unsigned char visible;
  unsigned char urgent;
  unsigned int x;
} Workspace;
typedef struct
{
  Workspace *workspaces;
  unsigned short size;
} Workspaces;
extern Workspaces workspaces;

extern int resize_mode;
#endif