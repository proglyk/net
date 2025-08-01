/**
	******************************************************************************
  * @file    net_srv.c
  * @author  Ilia Proniashin, PJSC Electrovipryamitel
  * @version V1.0.0
  * @date    26-Jule-2025
  * @brief   Implementation of network layer for STM32 platform
  * @attention The source code is presented as is
  *****************************************************************************/

#include "net_srv.h"
#include "stdlib.h"
#include "string.h"
#include "userlib.h"
#include "lwip/def.h"
#include "lwip/stats.h"
#include "userint.h"

// Объявление локальных макросов

#define SRV_ADDED(PX)                   PX->slTaskCounter++
#define RMT_CLT_ADDED(PX)               SRV_ADDED(PX)

// Прототипы локальных (static) функций

static void	 dispatcher( void * );
static void	 thread_pool( void * );
static s32_t setup( srv_ctx_t * );
static int   delete(srv_ctx_t *ctx);
static s32_t get_servers_number(net_srv_t *);
static void	 server_deleted_cb(void *);
static void  remote_client_deleted_cb(void *);

// Публичные (public) функции

/**	----------------------------------------------------------------------------
	* @brief ??? */
int
	net_srv__task_create(net_srv_t *px) {
/*----------------------------------------------------------------------------*/
	return xTaskCreate( dispatcher, 
                      "srv_disp", 
                      (3*configMINIMAL_STACK_SIZE), 
                      (void*)px, //FIXME
                      uxGetPriority(PRIO_NORM), 
                      &(px->pvDispHndl) );
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
bool
	net_srv__is_enabled(net_srv_t *px, const char *name) {
/*----------------------------------------------------------------------------*/
  srv_ctx_t *ctx;
  if (!px) return false;
  
  // поиск
  for (u8_t i = 0; i < SRV_NUM_MAX; i++) {
    ctx = (srv_ctx_t *)(px->axServers+i);
		if (ctx) {
			if ( !strcmp((const char *)ctx->acName, name) ) {
				return ctx->bEnable;
			}
		}
	}
  return false;
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
void
	net_srv__enable(net_srv_t *px, const char *name) {
/*----------------------------------------------------------------------------*/
  srv_ctx_t *ctx;
  if (!px) return;
  
  // поиск и редакт-ие
  for (u8_t i = 0; i < SRV_NUM_MAX; i++) {
    ctx = (srv_ctx_t *)(px->axServers+i);
		if (ctx) {
      // FIXME изначально неверное условие (см. "net_srv__disable")
			if ((!strcmp( (const char *)ctx->acName, name)) & (!ctx->bEnable) ) {
				// включаем
				ctx->bEnable = true;
			}
		}
	}
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
void
	net_srv__disable(net_srv_t *px, const char *name) {
/*----------------------------------------------------------------------------*/
  srv_ctx_t *ctx;
  if (!px) return;
  
  // поиск и редакт-ие
  for (u8_t i = 0; i < SRV_NUM_MAX; i++) {
    ctx = (srv_ctx_t *)(px->axServers+i);
		if (ctx->handle) {
			if ((!strcmp( (const char *)ctx->acName, name)) & (ctx->bEnable) ) {
				// удаляем сокеты удаленных клентов, Удаляем их задачи
				for (u8_t j=0; j<RMT_CLT_MAX; j++) {
					if ((ctx->axRmtClt+j)->handle)
            net_conn__del(ctx->axRmtClt+j, (ctx->axRmtClt+j)->handle);
				}
				// удаляем сокет сервера, Удаляем задачу
				delete(ctx);
				ctx->bEnable = false;
			}
		}
	}
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
void
	net_srv__delete_all(net_srv_t *px) {
/*----------------------------------------------------------------------------*/
  srv_ctx_t *ctx;
  if (!px) return;
  
  // поиск и редакт-ие
  for (u8_t i = 0; i < SRV_NUM_MAX; i++) {
    ctx = (srv_ctx_t *)(px->axServers+i);
		if (ctx->handle) {
      // удаляем сокет сервера, Удаляем задачу. Не трогаем bEnable
      delete(ctx);
		}
	}
}

// Локальные (static) функции

/**	----------------------------------------------------------------------------
	* @brief ??? */
static void
	server_deleted_cb(void *pv) {
/*----------------------------------------------------------------------------*/
  net_srv_t *px = (net_srv_t *)pv;
  px->slTaskCounter--;
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
static void
	remote_client_deleted_cb(void *pv) {
/*----------------------------------------------------------------------------*/
  srv_ctx_t *ctx = (srv_ctx_t *)pv;
  ctx->slTaskCounter--;
}

/**	----------------------------------------------------------------------------
	* @brief ???
	* @retval error: Статус выполнения функции. */
static s32_t
  get_servers_number(net_srv_t *px) {
/*----------------------------------------------------------------------------*/
	s32_t num = 0;
  if (!px) return -1;
  
  for (uint8_t i = 0; i < SRV_NUM_MAX; i++) {
    if (strcmp( (const char *)px->axServers[i].acName, "" ))
      num += 1;
	}
	return num;
}

/**	----------------------------------------------------------------------------
	* @brief ???
	* @retval error: Статус выполнения функции. */
static void
  dispatcher(void *pv) {
/*----------------------------------------------------------------------------*/
  BaseType_t rc;
  int status = 0;
  s32_t err;
  srv_ctx_t *ctx;
  net_srv_t *px = (net_srv_t *)pv;
  
  int size = get_servers_number(px);
  if (size < 0) return;
  
  do {
    for (int i=0; i<size; i++) {
      ctx = (srv_ctx_t *)(px->axServers + i);
      if ( (px->bNetifIsUp)&(!ctx->handle)&(ctx->bEnable) ) {
        err = setup(ctx);
        if (err >= 0) {
          ctx->pvDeleted = (task_cb_fn_t)server_deleted_cb;
          ctx->pvPld = (void *)px;
          ctx->bLatched = 0;
          // запуск отдельной задачи под каждого нового клиента
          rc = xTaskCreate( thread_pool, 
                            (const char *)ctx->acName,
                            (6*configMINIMAL_STACK_SIZE), 
                            (void *)ctx,
                            uxGetPriority(PRIO_NORM), 
                            &(ctx->handle) );
					// проверяем статус и выходим из цикла если !pdPASS
					if (rc == pdPASS) {
						SRV_ADDED(px);
					} else {
						status = -1;
            LWIP_DEBUGF( NET_DEBUG, ("can't create \"%s\", in '%s' /NET/net_srv.c:%d\r\n", 
              (const char *)pcTaskGetName(NULL), __FUNCTION__, __LINE__) );
					}   
					break;
        } else {
          if (!ctx->bLatched) {
            LWIP_DEBUGF( NET_DEBUG, ("Can't setup srv #3 (err=%d), in '%s' /NET/net_srv.c:%d\r\n", 
              err, __FUNCTION__, __LINE__) );
            ctx->bLatched = 1;
          }
          
        }
      }
    }
    // задержка перед ... чем?
    vTaskDelay(250);
  } while (status == 0);
    
  return;
}

/**	----------------------------------------------------------------------------
	* @brief  ???
	* @param  pctl: Указатель на всё */
static void
	thread_pool( void *pargv ) {
/*----------------------------------------------------------------------------*/
	s32_t sock;
	struct sockaddr_in remote;
	u32_t remotelen = sizeof(remote);
	char name[8];
  srv_ctx_t *ctx = (srv_ctx_t *)pargv;
  BaseType_t rc = pdPASS;
	if (!ctx) goto exit;

  LWIP_DEBUGF( NET_DEBUG, ("Server \"%s\" created, in '%s' /NET/net_srv.c:%d\r\n", 
              (const char *)pcTaskGetName(NULL), __FUNCTION__, __LINE__) );
	// Запускаем цикл идентификации очередного нового клиента до тех пор...
	do {
		// Создаем сокет для очередного нового клиента.
		sock = lwip_accept( ctx->slSockServ,
                        (struct sockaddr*)&remote,
                        (socklen_t*)&remotelen );
    //
    //stats_display();
		// Если сокет нового подкл. успешно открыт, то запускаем новую задачу клиента	
		if (sock >= 0) {
			for (int ig=0; ig<RMT_CLT_MAX; ig++) {
				if (!ctx->axRmtClt[ig].handle) {
					// полезная нагрузка в поля структуры, связанной конкретным клиентом
					// копируются из родительской структуры
          ctx->axRmtClt[ig].pvPld = (void *)ctx;
          ctx->axRmtClt[ig].pxFn = ctx->pxFn;
          ctx->axRmtClt[ig].pvTopPld = ctx->pvTopPld;
          // присваиваются здесь
          ctx->axRmtClt[ig].xData.slSock = sock;
          ctx->axRmtClt[ig].pvDeleted = (task_cb_fn_t)remote_client_deleted_cb;
					// даем имя строке
					sprintf(name, "clt%02d", ig);
					// запуск отдельной задачи под каждого нового клиента
					rc = xTaskCreate( net_conn__do,
                            name,
                            (6*configMINIMAL_STACK_SIZE), 
                            &(ctx->axRmtClt[ig]),
                            uxGetPriority(PRIO_NORM),
                            &(ctx->axRmtClt[ig].handle) );
          // проверяем статус и выходим из цикла если !pdPASS
					if (rc == pdPASS) {
						RMT_CLT_ADDED(ctx);
					} else {
            LWIP_DEBUGF( NET_DEBUG, ("can't create \"%s\", in '%s' /NET/net_srv.c:%d\r\n", 
              (const char *)pcTaskGetName(NULL), __FUNCTION__, __LINE__) );
					}   
					break;
				}
			}
		}
	// ... до тех пор, пока активны флаги MBTCP_OK или MBTCP_NOCLIENT. Иначе в 
	// случае флага MBTCP_LOSECON выйти из цикла.
	} while ( (sock >= 0) & (rc==pdPASS) );

exit:
  LWIP_DEBUGF( NET_DEBUG, ("Server \"%s\" deleted, in '%s' /NET/net_srv.c:%d\r\n", 
    (const char *)pcTaskGetName(NULL), __FUNCTION__, __LINE__) );
  // Закрыть ранее открытый сокет нового подключения, Удаляем запись о 
  // подключении, Удаляем задачу
  if (ctx->slSockServ != -1) {
    lwip_close(ctx->slSockServ);
  }
  if (ctx->pvDeleted) ctx->pvDeleted(ctx->pvPld);
  memset(ctx, 0, sizeof(conn_ctx_t));
  ctx->slSockServ = -1;
	vTaskDelete(NULL);
}

/**	----------------------------------------------------------------------------
	* @brief  ???
	* @param  pctl: Указатель на всё */
static s32_t
	setup( srv_ctx_t *psrv ) {
/*----------------------------------------------------------------------------*/
	int on=1;
	struct sockaddr_in local;

	// 1/?. Создаем общий сетевой сокет TCP.
	psrv->slSockServ = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (psrv->slSockServ < 0) {
		return -1; //TODO надо как-то обработать ошибку
	}
  
	//set master socket to allow multiple connections , this is just a good habit, it will work without this
	if(setsockopt(psrv->slSockServ, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) {
		goto gtFail;
	}
	
	// 2/?. Заполняем структуру со след.полями
	memset(&local, 0, sizeof(local));
	local.sin_family      = AF_INET;
	local.sin_port        = PP_HTONS(psrv->ulPort);
	
	// 3/?. bind to local address
	if (lwip_bind(psrv->slSockServ, (struct sockaddr *)&local, sizeof(local)) < 0) {
		goto gtFail;
	}
		
	// 4/?.
	if (lwip_listen(psrv->slSockServ, TCP_LISTEN_BACKLOG) != 0) {
		goto gtFail;
	}
	
	return 0;

	gtFail:
	// закрывать каждый раз, если успешно открывался
	lwip_close(psrv->slSockServ);
	return -1;  //TODO надо как-то обработать ошибку
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
static int
	delete(srv_ctx_t *ctx) {
/*----------------------------------------------------------------------------*/
  LWIP_DEBUGF( NET_DEBUG, ("task \"%s\" deleted, in '%s' /NET/net_srv.c:%d\r\n", 
    (const char *)pcTaskGetName(ctx->handle), __FUNCTION__, __LINE__) );
  
  // Закрыть ранее открытый сокет нового подключения, Удаляем запись о 
  // подключении, Удаляем задачу
  if (ctx->slSockServ != -1) {
    lwip_close(ctx->slSockServ);
  }
  if (ctx->pvDeleted) ctx->pvDeleted(ctx->pvPld);
  vTaskDelete(ctx->handle);
  // выключаем
  ctx->handle = NULL;
  ctx->slSockServ = -1;
  return 0;
}
