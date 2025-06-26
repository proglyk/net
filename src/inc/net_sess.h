#ifndef _NET_SESS_H_
#define _NET_SESS_H_

#include "net_if.h"
#include "userint.h"
#include "FreeRTOS.h"
#include "task.h"

typedef void (*task_cb_fn_t)(void *);

typedef struct {
  // ���������� �������
	TaskHandle_t handle;
  task_cb_fn_t pvDeleted;
  void *pvPld;
  // ����� ������, ������������ � ������ �� ������������ ����
	net_if_data_t xData;
  // ���������� �� ����� ����������� (����� ��������� �����)
  net_if_fn_t *pxFn;
} sess_ctx_t;

void net__session(void *);
int	 net_sess__delete(sess_ctx_t *ctx,TaskHandle_t);

#endif //_NET_SESS_H_