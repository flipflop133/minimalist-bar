#include "i3ipc.h"
#include "display.h"
#include <cjson/cJSON.h>
#include <fcntl.h>
#include <i3/ipc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static void parse_i3_workspaces(char *config);
static void parse_i3_modes(char *config);

Workspaces workspaces;
int resize_mode;
char *socket_path = NULL;

int retrieve_i3_sock_path()
{
  while (1)
  {
    FILE *fp = popen("i3 --get-socketpath", "r");
    if (fp != NULL)
    {
      socket_path = malloc(50);
      fgets(socket_path, 50, fp);
      if (strlen(socket_path) == 0)
      {
        continue;
      }
      socket_path[strlen(socket_path) - 1] = '\0';
      pclose(fp);
      return 0;
    }
    usleep(100000);
  }
  return -1;
}

int connect_to_ipc_socket()
{

  int sockfd;
  struct sockaddr_un addr;

  sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;

  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
  connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));

  return sockfd;
}

void send_ipc_message(int sockfd, int type, const char *payload)
{
  i3_ipc_header_t header;

  // Fill the header with appropriate values
  memcpy(header.magic, I3_IPC_MAGIC, sizeof(header.magic));
  header.size = payload != NULL ? strlen(payload) : 0;
  header.type = type;

  // Send the header
  write(sockfd, &header, sizeof(header));

  // Send the message content
  if (payload != NULL)
  {
    write(sockfd, payload, header.size);
  }
}

int receive_ipc_response(int sockfd)
{
  i3_ipc_header_t header;
  ssize_t bytes_read;

  // Receive the header
  if ((bytes_read = read(sockfd, &header, sizeof(header))) != sizeof(header))
  {
    perror("read");
    return -1;
  }

  char buffer[header.size + 1];

  read(sockfd, buffer, header.size);
  buffer[header.size] = '\0';
  int sockfd_workspace;

  if (header.type & (1UL << 31))
  {
    switch (header.type)
    {
    case I3_IPC_EVENT_WORKSPACE:
      sockfd_workspace = connect_to_ipc_socket();
      send_ipc_message(sockfd_workspace, I3_IPC_MESSAGE_TYPE_GET_WORKSPACES, NULL);
      receive_ipc_response(sockfd_workspace);
      break;
    case I3_IPC_EVENT_MODE:
      sockfd_workspace = connect_to_ipc_socket();
      send_ipc_message(sockfd_workspace, I3_IPC_MESSAGE_TYPE_GET_BINDING_STATE, NULL);
      receive_ipc_response(sockfd_workspace);
      break;
    default:
      break;
    }
  }
  else
  {
    switch (header.type)
    {
    case I3_IPC_REPLY_TYPE_WORKSPACES:
      parse_i3_workspaces(buffer);
      display_workspaces();
      break;

    case I3_IPC_REPLY_TYPE_GET_BINDING_STATE:
      parse_i3_modes(buffer);
      display_workspaces();
      break;
    default:
      break;
    }
  }
  return 0;
}

void *listen_to_i3(void *)
{
  retrieve_i3_sock_path();
  printf("%s\n", socket_path);

  int sockfd = connect_to_ipc_socket();

  send_ipc_message(sockfd, I3_IPC_MESSAGE_TYPE_SUBSCRIBE, "[ \"workspace\", \"mode\"]");

  // Retrieve workspaces at least one time for initialisation
  int sockfd_workspace = connect_to_ipc_socket();
  send_ipc_message(sockfd_workspace, I3_IPC_MESSAGE_TYPE_GET_WORKSPACES, NULL);
  receive_ipc_response(sockfd_workspace);

  while (1)
  {
    int response = receive_ipc_response(sockfd);
    if (response == -1)
    {
      // handle display changes. TODO: handle this in the right way, we lose connection to the socket on screen change and this should not happen
      sleep(1);
      close(sockfd);
      sockfd = connect_to_ipc_socket();
      send_ipc_message(sockfd, I3_IPC_MESSAGE_TYPE_SUBSCRIBE, "[ \"workspace\", \"mode\"]");
    }
  }
  close(sockfd);
  return 0;
}

static void parse_i3_workspaces(char *config)
{
  // Clean workspaces list
  free(workspaces.workspaces);
  workspaces.size = 0;

  // Parse json from config file
  cJSON *json = cJSON_Parse(config);
  cJSON *workspace = NULL;
  unsigned short i = 0;
  unsigned short workspaces_number = 10;

  workspaces.workspaces = malloc(workspaces_number * sizeof(Workspaces));
  cJSON_ArrayForEach(workspace, json)
  {
    cJSON *num = cJSON_GetObjectItemCaseSensitive(workspace, "num");
    cJSON *visible = cJSON_GetObjectItemCaseSensitive(workspace, "visible");
    cJSON *urgent = cJSON_GetObjectItemCaseSensitive(workspace, "urgent");
    cJSON *rect = cJSON_GetObjectItemCaseSensitive(workspace, "rect");
    cJSON *x = cJSON_GetObjectItemCaseSensitive(rect, "x");
    workspaces.workspaces[i].num = num->valueint;
    workspaces.workspaces[i].visible = visible->valueint;
    workspaces.workspaces[i].urgent = urgent->valueint;
    workspaces.workspaces[i].x = x->valueint;
    i++;
    if (i > workspaces_number)
    {
      workspaces_number += 10;
      workspaces.workspaces = realloc(workspaces.workspaces, workspaces_number * sizeof(Workspaces));
    }
  }
  workspaces.size = i;
  cJSON_Delete(json);
}

static void parse_i3_modes(char *config)
{
  // Parse json from config file
  cJSON *json = cJSON_Parse(config);
  cJSON *mode = cJSON_GetObjectItemCaseSensitive(json, "name");
  if (!strcmp("resize", mode->valuestring))
  {
    resize_mode = 1;
  }
  else
  {
    resize_mode = 0;
  }
  cJSON_Delete(json);
}