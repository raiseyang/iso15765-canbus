/// \file           : queue_demo.c
/// \author         : Dongsheng.Yang3
/// \brief          :
/// \date           : 2024/7/5

#include "lib_iqueue.h"
#include "stdio.h"

#define MAX_ELEMENTS_SIZE (100)

typedef struct {
  int a;
  int b;
} can_info_t;

static iqueue_t queue;
static uint8_t queue_buf[MAX_ELEMENTS_SIZE * sizeof(can_info_t)];

int main() {
  printf("hello queue demo.");
  printf("初始化 iqueue_init");
  iqueue_init(&queue, MAX_ELEMENTS_SIZE, sizeof(can_info_t), queue_buf);

  size_t size;
  iqueue_size(&queue, &size);
  printf("获取当前有多少个元素 iqueue_size=%zu\n", size);

  for (int i = 0; i < 10; i++) {
    can_info_t can_info = {
        .a = i,
        .b = i + 1,
    };
    iqueue_enqueue(&queue, &can_info);
    printf("入队元素 i=%d\n", i);
  }

  iqueue_size(&queue, &size);
  printf("获取当前有多少个元素 iqueue_size=%zu\n", size);

  for (int i = 0; i < 10; i++) {
    can_info_t can_info;
    iqueue_dequeue(&queue, &can_info);
    printf("出队元素 i=%d\n", i);
  }

    iqueue_size(&queue, &size);
    printf("获取当前有多少个元素 iqueue_size=%zu\n", size);
  return 0;
}