#include "network.h"
#include "../defs.h"
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
#include "../display.h"
void determine_wifi_status(unsigned int flags);
static void retrieve_initial_status(void);
static char *convert_signal_to_icon(int signal);
int wifi_signal = 1; // TODO : read this value from config file

#define DEFAULT_INTERFACE "wlan0" // TODO : read this value from config file

static void execute_ioctl_command(int command, char *result) {
  struct iwreq wreq;
  sprintf(wreq.ifr_ifrn.ifrn_name, IW_INTERFACE);
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  // Retrieve the SSID
  if (command == SIOCGIWESSID) {
    wreq.u.essid.pointer = malloc(IW_ESSID_MAX_SIZE + 1);
    wreq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
    ioctl(sockfd, command, &wreq);
    strcpy(result, wreq.u.essid.pointer);
    free(wreq.u.essid.pointer);
  }
  // Retrieve the signal level
  else if (command == SIOCGIWSTATS) {
    struct iw_statistics stats;
    wreq.u.data.pointer = &stats;
    wreq.u.data.length = sizeof(struct iw_statistics);
    ioctl(sockfd, command, &wreq);
    strcpy(result, convert_signal_to_icon(stats.qual.qual));
  }

  close(sockfd);
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

void *wifi_update(void *) {
  pthread_mutex_lock(&mutex);
  modules[network].string = (char *)malloc((NETWORK_BUFFER * sizeof(char)));
  modules[network].string[0] = '\0';
  pthread_mutex_unlock(&mutex);
  retrieve_initial_status();

  struct sockaddr_nl sa;

  memset(&sa, 0, sizeof(sa));
  sa.nl_family = AF_NETLINK;
  sa.nl_groups = RTMGRP_LINK;
  int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  bind(fd, (struct sockaddr *)&sa, sizeof(sa));
  char buffer[4096];
  struct nlmsghdr *nlh;
  ssize_t len;

  while (1) {
    len = recv(fd, buffer, sizeof(buffer), 0);
    if (len < 0) {
      perror("recv");
      exit(EXIT_FAILURE);
    }
    for (nlh = (struct nlmsghdr *)buffer; NLMSG_OK(nlh, len);
         nlh = NLMSG_NEXT(nlh, len)) {
      if (nlh->nlmsg_type == RTM_NEWLINK || nlh->nlmsg_type == RTM_DELLINK) {
        determine_wifi_status(((struct ifinfomsg *)NLMSG_DATA(nlh))->ifi_flags);
      }
    }
  }
  free(modules[network].string);
  return NULL;
}

void determine_wifi_status(unsigned int flags) {
  char interface_name[20] = {0};
  if ((flags & IFF_UP) && (flags & IFF_RUNNING)) {
    // get_interface_name(interface_name);
    execute_ioctl_command(SIOCGIWESSID, interface_name);
    if (wifi_signal) {
      // get_wifi_quality();
      execute_ioctl_command(SIOCGIWSTATS, modules[network].string);
    } else {
      strcpy(modules[network].string, "󰖩 ");
    }
    strcat(modules[network].string, interface_name);
  } else if (flags & IFF_UP) {
    strcpy(modules[network].string, "󰖩 ");
  } else {
    strcpy(modules[network].string, "󰖪 ");
  }
  display_modules(modules[network].position);
}

static void retrieve_initial_status(void) {
  int socId = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  struct ifreq if_req;
  (void)strncpy(if_req.ifr_name, DEFAULT_INTERFACE, sizeof(if_req.ifr_name));
  ioctl(socId, SIOCGIFFLAGS, &if_req);
  determine_wifi_status(if_req.ifr_flags);
  close(socId);
}