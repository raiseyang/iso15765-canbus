/// \file           : lib_utils.c
/// \author         : Dongsheng.Yang3
/// \brief          :
/// \date           : 2024/7/14

#include "lib_utils.h"
#include "stdio.h"

void log_indn(n_indn_t *info){

  printf("fr_fmt: %s,   rslt: %d, msg_sz: %u \n",
         info->fr_fmt == 0x01 ? "CLASS CAN" : "CAN FD",
         info->rslt, info->msg_sz);

}