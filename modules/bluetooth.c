#include "bluetooth.h"
#include "../defs.h"
#include "../display.h"
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
void *bluetooth_update(void *arg) {
  printf("enter bluetooth_update\n");
  struct Module *module = (struct Module *)arg;
  printf("ok bluetooth_update\n");
  pthread_mutex_lock(&mutex);
  module->string = (char *)malloc((BLUETOOTH_BUFFER * sizeof(char)));
  module->string[0] = '\0';
  pthread_mutex_unlock(&mutex);
  int dev_id = hci_get_route(NULL);
  int sock = hci_open_dev(dev_id);
  struct hci_version ver;

  while (running) {
    if (hci_read_local_version(sock, &ver, 0) < 0) {
      strcpy(module->string, "󰂲");
    } else {
      struct hci_conn_list_req *cl;
      struct hci_conn_info *ci;

      cl = (struct hci_conn_list_req *)malloc(16 * sizeof(uint8_t) + 1);
      cl->dev_id = dev_id;
      cl->conn_num = 16;

      ioctl(sock, HCIGETCONNLIST, (void *)cl);
      if (cl->conn_num > 0) {
        char remote_device_address[1024];
        for (int i = 0; i < cl->conn_num; i++) {
          ci = &cl->conn_info[i];
          ba2str(&ci->bdaddr, remote_device_address);
          char *name = (char *)malloc(30 * sizeof(char));
          hci_read_remote_name(sock, &ci->bdaddr, sizeof(name), name, 0);
          sprintf(module->string, "󰂱 %s", name);
          free(name);
        }
      } else {
        strcpy(module->string, "󰂯");
      }
      free(cl);
    }
    sleep(1);
  }
  printf("bt calling display_modules\n");
  display_modules(module->position);
  close(sock);
  free(module->string);
  return 0;
}