#include "network.h"
#include "../defs.h"
#include "../display/display.h"
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/wireless.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
void determine_wifi_status(unsigned int flags, int interface_type, struct Module *module);
static void retrieve_initial_status(int interface_type, struct Module *module);
static char *convert_signal_to_icon(int signal);
int wifi_signal = 1; // TODO : read this value from config file

static int isWiFiInterface(const char *ifname) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct iwreq wrq;
    memset(&wrq, 0, sizeof(struct iwreq));
    strncpy(wrq.ifr_name, ifname, IFNAMSIZ - 1);

    int result = ioctl(sock, SIOCGIWMODE, &wrq);

    close(sock);

    return (result != -1) ? 1 : 0;
}

static int execute_ioctl_command(int command, char *result, struct Module *module) {
  struct iwreq wreq;
  sprintf(wreq.ifr_ifrn.ifrn_name, ((struct Network*)(module->Module_infos))->interface);
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  // Retrieve the SSID
  if (command == SIOCGIWESSID) {
    wreq.u.essid.pointer = malloc(IW_ESSID_MAX_SIZE + 1);
    wreq.u.essid.length = IW_ESSID_MAX_SIZE + 1;

    if(ioctl(sockfd, command, &wreq) == -1) {
      return 1;
    }
    strcpy(result, wreq.u.essid.pointer);
    free(wreq.u.essid.pointer);
  }
  // Retrieve the signal level
  else if (command == SIOCGIWSTATS) {
    struct iw_statistics stats;
    wreq.u.data.pointer = &stats;
    wreq.u.data.length = sizeof(struct iw_statistics);
    if(ioctl(sockfd, command, &wreq) == -1) {
      return 1;
    }
    strcpy(result, convert_signal_to_icon(stats.qual.qual));
  }

  close(sockfd);
  return 0;
}

static char *convert_signal_to_icon(int signal) {
  // Determine the quality level and select the icon
  if (signal >= 70) {
    return "󰤨 ";
  } else if (signal >= 50) {
    return "󰤥 ";
  } else if (signal >= 30) {
    return "󰤢 ";
  } else if (signal >= 20) {
    return "󰤟 ";
  } else {
    return "󰤯 ";
  }
}

void *wifi_update(void *arg) {
  int interface_type = ETHERNET;
  struct Module *module = (struct Module *)arg;

  // Determine interface type
  if(isWiFiInterface(((struct Network*)(module->Module_infos))->interface)){
    interface_type = WIFI;
  }

  retrieve_initial_status(interface_type, module);

  struct sockaddr_nl sa;

  memset(&sa, 0, sizeof(sa));
  sa.nl_family = AF_NETLINK;
  sa.nl_groups = RTMGRP_LINK;
  int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  bind(fd, (struct sockaddr *)&sa, sizeof(sa));
  char buffer[4096];
  struct nlmsghdr *nlh;
  ssize_t len;

  while (running) {
    len = recv(fd, buffer, sizeof(buffer), 0);
    if (len < 0) {
      perror("recv");
      exit(EXIT_FAILURE);
    }
    for (nlh = (struct nlmsghdr *)buffer; NLMSG_OK(nlh, len);
         nlh = NLMSG_NEXT(nlh, len)) {
      if (nlh->nlmsg_type == RTM_NEWLINK || nlh->nlmsg_type == RTM_DELLINK) {
        determine_wifi_status(((struct ifinfomsg *)NLMSG_DATA(nlh))->ifi_flags, interface_type, module);
      }
    }
  }
  free(module->string);
  return NULL;
}

void determine_wifi_status(unsigned int flags, int interface_type, struct Module *module) {
  char interface_name[20] = {0};
  if ((flags & IFF_UP) && (flags & IFF_RUNNING) && (interface_type == WIFI)) {
    // get_interface_name(interface_name);
    execute_ioctl_command(SIOCGIWESSID, interface_name, module);
    if (wifi_signal) {
      // get_wifi_quality();
      execute_ioctl_command(SIOCGIWSTATS, module->string, module);
    } else {
      strcpy(module->string, "󰖩 ");
    }
    strcat(module->string, interface_name);
  } else if (flags & IFF_UP) {
    if(interface_type == WIFI){
      strcpy(module->string, "󰖩 ");
    }
    else{
      strcpy(module->string, "󰈁 ");
    }

  } else {
    if(interface_type == WIFI){
      strcpy(module->string, "󰖪 ");
    }
    else {
      strcpy(module->string, "󰈂 ");
    }
  }
  display_modules(module->position);
}

static void retrieve_initial_status(int interface_type, struct Module *module) {
  int socId = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  struct ifreq if_req;
  (void)strncpy(if_req.ifr_name, ((struct Network*)(module->Module_infos))->interface, sizeof(if_req.ifr_name));
  ioctl(socId, SIOCGIFFLAGS, &if_req);
  determine_wifi_status(if_req.ifr_flags, interface_type, module);
  close(socId);
}