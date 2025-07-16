#ifndef _NET_SESS_H_
#define _NET_SESS_H_

#include "net_if.h"
#include "userint.h"
#include "FreeRTOS.h"
#include "task.h"

typedef void (*task_cb_fn_t)(void *);

typedef struct {
  // управление задачей
	TaskHandle_t handle;
  task_cb_fn_t pvDeleted;
  void *pvPld;
  // некие данные, передаваемые в задачу из запускающего кода
	net_if_data_t xData;
  // переменные на новое подключение (новый удаленный клент)
  net_if_fn_t *pxFn;
} sess_ctx_t;

void net__session(void *);
int	 net_sess__delete(sess_ctx_t *ctx,TaskHandle_t);

#endif //_NET_SESS_H_