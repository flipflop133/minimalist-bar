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

Workspaces workspaces;

void retrieve_i3_socket_path(char *path) {
  FILE *fp = popen("i3 --get-socketpath", "r");

  if (fp != NULL) {
    fgets(path, 50, fp);
    path[strlen(path) - 1] = '\0';
    pclose(fp);
  }
}

int connect_to_ipc_socket() {
  int sockfd;
  struct sockaddr_un addr;

  sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  char path[50];
  retrieve_i3_socket_path(path);
  strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

  connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));

  return sockfd;
}

void send_ipc_message(int sockfd, int type, const char *payload) {
  i3_ipc_header_t header;
  ssize_t len = strlen(payload);

  // Fill the header with appropriate values
  memcpy(header.magic, I3_IPC_MAGIC, sizeof(header.magic));
  header.size = len;
  header.type = type;

  // Send the header
  write(sockfd, &header, sizeof(header));

  // Send the message content
  write(sockfd, payload, len);
}

void receive_ipc_response(int sockfd, int type) {
  i3_ipc_header_t header;
  ssize_t bytes_read;

  // Receive the header
  if ((bytes_read = read(sockfd, &header, sizeof(header))) != sizeof(header)) {
    perror("read");
    close(sockfd);
    exit(1);
  }
  char buffer[header.size + 1];

  read(sockfd, buffer, header.size);

  buffer[header.size] = '\0';
  if (type == I3_IPC_MESSAGE_TYPE_GET_WORKSPACES) {
    parse_i3_workspaces(buffer);
  }
}

void *listen_to_i3(void *) {
  int sockfd = connect_to_ipc_socket();
  int sockfd_workspace = connect_to_ipc_socket();

  send_ipc_message(sockfd, I3_IPC_MESSAGE_TYPE_SUBSCRIBE, "[ \"workspace\"]");
  while (1) {
    receive_ipc_response(sockfd, I3_IPC_MESSAGE_TYPE_SUBSCRIBE);
    send_ipc_message(sockfd_workspace, I3_IPC_MESSAGE_TYPE_GET_WORKSPACES, "");
    receive_ipc_response(sockfd_workspace, I3_IPC_MESSAGE_TYPE_GET_WORKSPACES);
    display_workspaces();
  }
  close(sockfd);
  return 0;
}

static void parse_i3_workspaces(char *config) {
  // Clean workspaces list
  free(workspaces.workspaces);
  workspaces.size = 0;

  // Parse json from config file
  cJSON *json = cJSON_Parse(config);
  cJSON *workspace = NULL;
  unsigned short i = 0;
  unsigned short  workspaces_number = 10;
  
  workspaces.workspaces = malloc(workspaces_number * sizeof(Workspaces));
  cJSON_ArrayForEach(workspace, json) {
    cJSON *num = cJSON_GetObjectItemCaseSensitive(workspace, "num");
    cJSON *visible = cJSON_GetObjectItemCaseSensitive(workspace, "visible");
    cJSON *urgent = cJSON_GetObjectItemCaseSensitive(workspace, "urgent");
    workspaces.workspaces[i].num = num->valueint;
    workspaces.workspaces[i].visible = visible->valueint;
    workspaces.workspaces[i].urgent = urgent->valueint;
    i++;
    if(i > workspaces_number){
      workspaces_number += 10;
      workspaces.workspaces = realloc(workspaces.workspaces, workspaces_number * sizeof(Workspaces));
    }
  }
  workspaces.size = i;
  cJSON_Delete(json);
}