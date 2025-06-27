#ifndef _NET_H_
#define _NET_H_

#include "net_clt.h"
#include "net_srv.h"
#include "userint.h"

typedef enum {
  CLT_TCP,  // = SOCK_STREAM,
  CLT_UDP   // = SOCK_DGRAM,
} socktype_t;

// Секция init

typedef struct {
  // Общие поля
  const char  *pcName;
  net_if_fn_t *pxFn;
  u32_t        ulPort;
  socktype_t   eSockType;
  bool         bEnabled;
  // Поля только для клиента
  u32_t        ulId;
  u8_t        *pcRmt;
  // ПОля только для сервера
  // ...
} net_init_t;

typedef struct {  
  // Диспетчер добавленных к проекту клиентов
  net_clt_t xClt;
  // Диспетчер добавленных к проекту клиентов
  net_srv_t xSrv;
} net_t;


// Прототипы публичных (public) функций

s32_t  net__init(net_t *);
void   net__run(net_t *);
s32_t  net__add_srv(net_t *, net_init_t *);
s32_t  net__add_clt(net_t *, net_init_t *);
void   net__input( net_t * );
net_t *net__inst(void);
void   net__irq( net_t * );

#endif