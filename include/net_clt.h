#ifndef _NET_CLT_H_
#define _NET_CLT_H_

#include "proj_conf.h"
#include "net_if.h"
#include "net_conn.h"
#include "lwip/sockets.h"
#include <stdbool.h>

typedef struct {  
  // внутренние
  struct       sockaddr_in remote;
  u8_t         acName[16];
  s32_t        ulSockType;
  bool         bEnable;
  // интерфейсные
  u32_t        ulId;
  u32_t        ulPort; //remote
  ip_addr_t    xIpRmt;
  const u8_t  *pcIpRmt;
  // переменные на программный стек клиента.
  // один стек - один сервер - одно подключение (один удаленный сервер)
  conn_ctx_t   xRmtSrv;
  net_if_fn_t *pxFn;
} clt_ctx_t;

typedef struct {
  // идентификаторы серверов tcp
  TaskHandle_t pvCltHndl;
  bool         bNetifIsUp;
  clt_ctx_t    axClients[CLT_NUM_MAX];
} net_clt_t;

// Прототипы публичных (public) функций
int	net_clt__task_create(net_clt_t *);

#endif //_NET_CLT_H_