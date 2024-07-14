/// \file           : cansocket.h
/// \author         : Dongsheng.Yang3
/// \brief          :
/// \date           : 2024/7/10

#ifndef CANBUS_CANSOCKET_H
#define CANBUS_CANSOCKET_H

#include <linux/can.h>
#include <linux/can/raw.h>
#include <stdio.h>
void init_can_socket(int *socket_id);

void send_can(int sock, const struct can_frame *frame);
ssize_t receive_can(int sock, struct can_frame *frame);
#endif // CANBUS_CANSOCKET_H
