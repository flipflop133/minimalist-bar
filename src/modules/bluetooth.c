#include "bluetooth.h"
#include "../defs.h"
#include "../display/display.h"
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

void *bluetooth_update(void *arg)
{
  struct Module *module = (struct Module *)arg;

  while (running)
  {
    int dev_id = hci_get_route(NULL);
    int sock = hci_open_dev(dev_id);
    struct hci_version ver;
    if (hci_read_local_version(sock, &ver, 0) < 0)
    {
      strcpy(module->string, "󰂲");
    }
    else
    {
      struct hci_conn_list_req *conn_list;
      struct hci_conn_info *ci;

      int max_conn = 10;
      conn_list = (struct hci_conn_list_req *)malloc(max_conn * (sizeof(struct hci_conn_list_req) + sizeof(struct hci_conn_info)));
      if(!conn_list){
        printf("Failed to allocate connection list memory\n.");
        continue;
      }
      conn_list->dev_id = dev_id;
      conn_list->conn_num = max_conn;

      if(ioctl(sock, HCIGETCONNLIST, (void *)conn_list)){
        printf("Couldn't get connection list.\n");
        continue;
      }

      if (conn_list->conn_num > 0)
      {
        for (int i = 0; i < conn_list->conn_num; i++)
        {
          ci = &conn_list->conn_info[i];
          char *name = (char *)malloc(BLUETOOTH_NAME * sizeof(char));
          if(!name){
            printf("Failed to allocate bluetooth name memory\n.");
            continue;
          }
          if (hci_read_remote_name(sock, &ci->bdaddr, BLUETOOTH_NAME, name, 0) < 0)
            strcpy(module->string, "󰂱 unknown");
          else
            sprintf(module->string, "󰂱 %s", name);
          free(name);
        }
      }
      else
      {
        strcpy(module->string, "󰂯");
      }
      free(conn_list);
    }
    display_modules(module->position);
    close(sock);
    sleep(1); // TODO don't sleep
  }

  free(module->string);
  return 0;
}