
#include "lib_iso15765.h"
#include "pthread.h"
#include "sys/time.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cansocket.h"

#include "lib_utils.h"

void Sleep(int millseconds) { usleep(1000 * millseconds); }

uint32_t GetTickCount() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (long long)tv.tv_sec * 1000LL + (long long)tv.tv_usec / 1000LL;
}

/******************************************************************************
 * Declaration | Static Functions
 ******************************************************************************/

static uint8_t send_frame1(cbus_id_type id_type, uint32_t id,
                           cbus_fr_format fr_fmt, uint8_t dlc, uint8_t *dt);
static void indn1(n_indn_t *info);
static void on_error(n_rslt err_type);
static uint32_t getms();

/******************************************************************************
 * Enumerations, structures & Variables
 ******************************************************************************/

static iso15765_t recv_handler = {.addr_md = N_ADM_FIXED,
                                  .fr_id_type = CBUS_ID_T_EXTENDED,
                                  .clbs.send_frame = send_frame1,
                                  .clbs.on_error = on_error,
                                  .clbs.get_ms = getms,
                                  .clbs.indn = indn1,
                                  .config.stmin = 0x3,
                                  .config.bs = 0x0f,
                                  .config.n_bs = 100,
                                  .config.n_cr = 3};

n_req_t frame1 = {
    .n_ai.n_pr = 0x07,
    .n_ai.n_sa = 0x11,
    .n_ai.n_ta = 0x22,
    .n_ai.n_ae = 0x00,
    .n_ai.n_tt = N_TA_T_PHY,
    .fr_fmt = CBUS_FR_FRM_STD,
    .msg = {0},
    .msg_sz = 0,
};

/******************************************************************************
 * Definition  | Static Functions
 ******************************************************************************/
uint16_t f_sz1 = 1;

int can_socket_id = -1;

uint16_t use_fr_fmt = CBUS_FR_FRM_STD;

static void rand_string(char *str, size_t size) {
  const char charset[] = "abcdefghijklmnopqrstuvwxyz";

  for (size_t n = 0; n < size; n++) {
    int key = rand() % ((int)sizeof(charset) - 1);
    str[n] = charset[key];
  }
}

static uint32_t getms() {
  return GetTickCount();
  ;
}

static void print_frame(uint8_t instance, uint8_t tp_mode, cbus_id_type mode,
                        uint32_t id, uint8_t ctp_ft, uint8_t dlc, uint8_t *dt) {
  printf(" (%d) - %d | TpMode: [%02x] | FrameType: [%2d]   IdType: [%d]   Id: "
         "[%8x]   DLC: [%02d]\t\t",
         instance, GetTickCount(), tp_mode, mode, ctp_ft, id, dlc);

  for (int s = 0; s < (int)dlc; s += 8) {
    int elements = (int)dlc - s > 8 ? 8 : (int)dlc - s;

    for (int i = s; i < (s + 8); i++) {
      printf((i < (s + elements)) ? "%02x " : "   ", dt[i]);
    }
    printf("\t");
    if ((s + elements) == (int)dlc) {
      printf("\n");
    } else {
      printf("\n     |\t\t\t\t\t\t\t\t\t\t\t\t\t");
    }
  }
}

static uint8_t send_frame1(cbus_id_type id_type, uint32_t id,
                           cbus_fr_format fr_fmt, uint8_t dlc, uint8_t *dt) {
  print_frame(1, 0x14, id_type, id, fr_fmt, dlc, dt);

  if (can_socket_id == -1) {
    printf("init can socket id \n");
    init_can_socket(&can_socket_id);
  }

  struct can_frame can_frame1 = {
      .can_id = id,
      .can_dlc = dlc, // frame payload length in byte (0 .. CAN_MAX_DLEN)
  };
  memcpy(can_frame1.data, dt, dlc);
  send_can(can_socket_id, &can_frame1);

  //        canbus_frame_t frame = { .id = id, .dlc = dlc, .id_type = id_type,
  //        .fr_format= fr_fmt }; memmove(frame.dt, dt, dlc);
  //        iso15765_enqueue(&handler2, &frame);
  return 0;
}

static void on_error(n_rslt err_type) {
  printf("ERROR OCCURED!:%04x\n", err_type);
}

static void indn1(n_indn_t *info) {

  log_indn(info);

  frame1.msg_sz = f_sz1;
  rand_string(frame1.msg, frame1.msg_sz);
  f_sz1 = f_sz1 == 512 ? 0 : f_sz1 + 1;
  iso15765_send(&recv_handler, &frame1);
}
/******************************************************************************
 * Definition  | Public Functions
 ******************************************************************************/

void loop_read_can() {

  struct can_frame can_frame1;
  printf("loop_read_can start. \n");

  if (can_socket_id == -1) {
    init_can_socket(&can_socket_id);
  }

  while (true) {
    if (can_socket_id != -1) {
      ssize_t ret = receive_can(can_socket_id, &can_frame1);
      if (ret > 0) {

        canbus_frame_t canbusFrame = {
            .id = can_frame1.can_id,
            .dlc = can_frame1.can_dlc,
            .fr_format = use_fr_fmt,
            .id_type = can_frame1.can_id & CAN_RTR_FLAG
                           ? CBUS_ID_T_EXTENDED
                           : CBUS_ID_T_STANDARD, // RTR
        };
        memcpy(canbusFrame.dt, can_frame1.data, can_frame1.can_dlc);

        iqueue_enqueue(&recv_handler.inqueue, &canbusFrame);
      }
    }
  }
}

int main() {
  printf("main start.\n");
  iso15765_init(&recv_handler);
  // 创建一个线程，读取can0上的消息，放在队列里面
  pthread_t t1;
  pthread_create(&t1, NULL, (void *(*)(void *))loop_read_can, NULL);
  // 处理 in out 消息
  while (1) {
    iso15765_process(&recv_handler);
  }

  pthread_join(t1, NULL);
  return 0;
}
