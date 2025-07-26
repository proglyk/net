/**
	******************************************************************************
  * @file    net_if.h
  * @author  Ilia Proniashin, PJSC Electrovipryamitel
  * @version V0.31.0
  * @date    26-Jule-2025
  * @brief   Implementation of network layer for STM32 platform
  * @attention The source code is presented as is
  *****************************************************************************/

#ifndef _PROTO_IF_H_
#define _PROTO_IF_H_

#include "userint.h"
#include "stdlib.h"
#include "lwip/sockets.h"

typedef struct {
  u32_t ulId;
  s32_t slSock;
  ip_addr_t xAddrRmt;
  u32_t ulPortRmt;
} net_if_data_t;

// upper init
typedef void  (*upper_init_ptr_t)(void **);
// sess init callback
typedef void  (*sess_init_cb_ptr_t)(void *);
// sess init
typedef void* (*sess_init_ptr_t)(sess_init_cb_ptr_t, net_if_data_t *, void *);

// sess do
typedef	s32_t (*sess_do_ptr_t)(void *);
// sess delete callback
typedef void  (*sess_del_cb_ptr_t)(void);
// sess delete
typedef	void  (*sess_del_ptr_t)(sess_del_cb_ptr_t, void *);


typedef struct {
  upper_init_ptr_t    pvUpperInit;
  void*               pvUpper;
  sess_init_ptr_t     ppvSessInit;
  sess_init_cb_ptr_t  ppvSessInitCb;
  sess_do_ptr_t       pslSessDo;
  sess_del_ptr_t      pvSessDel;
  sess_del_cb_ptr_t   pvSessDelCb;
} net_if_fn_t;

int net_if__copy(net_if_fn_t *, net_if_fn_t *);

#endif //_PROTO_IF_H_