/// \file           : cansocket.c
/// \author         : Dongsheng.Yang3
/// \brief          :
/// \date           : 2024/7/10
#include "cansocket.h"
#include <errno.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

void init_can_socket(int *socket_id) {
  int sock;
  sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (sock < 0) {
    printf("error.");
    return;
  }

  struct sockaddr_can addr;
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, "can0", IFNAMSIZ - 1);
  ioctl(sock, SIOCGIFINDEX, &ifr);
  addr.can_ifindex = ifr.ifr_ifindex;
  addr.can_family = AF_CAN;
  if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    printf("Error binding socket.");
    return;
  }
  *socket_id = sock;
}

void send_can(int sock, const struct can_frame *frame) {
  ssize_t len = write(sock, frame, sizeof(struct can_frame));
  if (len != sizeof(*frame)) {
    printf("error=%s ", strerror(errno));
  }
}

ssize_t receive_can(int sock, struct can_frame *frame) {
  ssize_t len = read(sock, frame, sizeof(struct can_frame));
  if (len <= 0) {
    printf("error=%s ", strerror(errno));
  }
  return len;
}