#ifndef _NET_SRV_H_
#define _NET_SRV_H_

#include "proj_conf.h"
#include "net_if.h"
#include "net_sess.h"
#include "lwip/sockets.h"
#include <stdbool.h>


typedef enum {
  SRV_TCP = SOCK_STREAM
} srv_type_t;

typedef struct {
  // управление задачей
	TaskHandle_t handle;
  task_cb_fn_t pvDeleted;
  void *pvPld;
  bool bEnable;
  bool bLatched;  // защелка только для случая неудачного вызова setup
  // данные
	s32_t slSockServ;
  u32_t ulPort;
  //struct sockaddr_in remote;
  u8_t acName[16];
  int slTaskCounter;
  srv_type_t eType;

  // переменные на программный стек сервера в целом.
  // один стек - один сервер - много подключений (много удаленных клиентов)
  //TaskHandle_t 	apvRmtCltHndl[RMT_CLT_MAX];
  sess_ctx_t    axRmtClt[RMT_CLT_MAX];
  
  net_if_fn_t   *pxFn;
} srv_ctx_t;

typedef struct {
  // идентификаторы серверов tcp
  bool bRunned;
  TaskHandle_t pvDispHndl;
  bool bNetifIsUp;
  //TaskHandle_t 	apvSrvHndl[SERVERS];
  int slTaskCounter;
  srv_ctx_t axServers[SRV_NUM_MAX];
} net_srv_t;

int	 net_srv__task_create(net_srv_t *);
void net_srv__enable(net_srv_t *, const char *);
void net_srv__disable(net_srv_t *, const char *);
bool net_srv__is_enabled(net_srv_t *, const char *);
void net_srv__delete_all(net_srv_t *);

#endif //_NET_SRV_H_