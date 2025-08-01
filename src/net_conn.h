/**
	******************************************************************************
  * @file    net_conn.h
  * @author  Ilia Proniashin, PJSC Electrovipryamitel
  * @version V1.0.0
  * @date    26-Jule-2025
  * @brief   Implementation of network layer for STM32 platform
  * @attention The source code is presented as is
  *****************************************************************************/

#ifndef _NET_CONN_H_
#define _NET_CONN_H_

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
  void *pvTopPld;
  // переменные на новое подключение (новый удаленный клент)
  net_if_fn_t *pxFn;
} conn_ctx_t;

// Прототипы публичных (public) функций
void net_conn__do(void *);
int	 net_conn__del(conn_ctx_t *, TaskHandle_t);

#endif //_NET_CONN_H_