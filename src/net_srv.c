#include "net_srv.h"
#include "stdlib.h"
#include "string.h"
#include "cmsis_os.h"
#include "lwip/def.h"
#include "lwip/stats.h"
#include "userint.h"

static void	dispatcher( void * );
static void	poll( void * );
static s32_t setup( srv_ctx_t * );
static void	remote_client_added(srv_ctx_t *);
static void remote_client_deleted_cb(void *);
static void	server_added(net_srv_t *);
static void	server_deleted_cb(void *);
static int delete(srv_ctx_t *ctx);
static s32_t get_servers_number(net_srv_t *);

/**	----------------------------------------------------------------------------
	* @brief ??? */
static void
	server_added(net_srv_t *px) {
/*----------------------------------------------------------------------------*/
  px->slTaskCounter++;
}

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
	remote_client_added(srv_ctx_t *ctx) {
/*----------------------------------------------------------------------------*/
  ctx->slTaskCounter++;
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
	* @brief ??? */
int
	net_srv__task_create(net_srv_t *px) {
/*----------------------------------------------------------------------------*/
	return xTaskCreate( dispatcher, 
                      "srv_disp", 
                      (3*configMINIMAL_STACK_SIZE), 
                      (void*)px, //FIXME
                      makeFreeRtosPriority(osPriorityNormal), 
                      &(px->pvDispHndl) );
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
bool
	net_srv__is_enabled(net_srv_t *px, const char *name) {
/*----------------------------------------------------------------------------*/
  srv_ctx_t *ctx;
  // пров арг-ов
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
  // пров арг-ов
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
  // пров арг-ов
  if (!px) return;
  // поиск и редакт-ие
  for (u8_t i = 0; i < SRV_NUM_MAX; i++) {
    ctx = (srv_ctx_t *)(px->axServers+i);
		if (ctx->handle) {
			if ((!strcmp( (const char *)ctx->acName, name)) & (ctx->bEnable) ) {
				// удаляем сокеты удаленных клентов, Удаляем их задачи
				for (u8_t j=0; j<RMT_CLT_MAX; j++) {
					if ((ctx->axRmtClt+j)->handle)
            net_sess__delete(ctx->axRmtClt+j, (ctx->axRmtClt+j)->handle);
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
  // пров арг-ов
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

static void dispatcher(void *pv) {
  BaseType_t rc;
  int status = 0;
  s32_t err;
  srv_ctx_t *ctx;
  net_srv_t *px = (net_srv_t *)pv;
  
  //int size = sizeof(px->axServers)/sizeof(srv_ctx_t);
  int size = get_servers_number(px);
  if (size < 0) return;
  
  px->bRunned = true;
  // TODO переместить в net
  //memset((void *)px->axServers, 0, SERVERS*sizeof(srv_ctx_t));
  
  do {
    for (int i=0; i<size; i++) {
      ctx = (srv_ctx_t *)(px->axServers + i);
      if ( (px->bNetifIsUp)&(!ctx->handle)&(ctx->bEnable) ) {
        //err = try_connect(ctx);
        if (i==2) {
          asm("nop");
        }
        err = setup(ctx);
        if (err >= 0) {
          ctx->pvDeleted = (task_cb_fn_t)server_deleted_cb;
          ctx->pvPld = (void *)px;
          ctx->bLatched = 0;
          // запуск отдельной задачи под каждого нового клиента
          rc = xTaskCreate( poll, 
                            (const char *)ctx->acName,
                            (6*configMINIMAL_STACK_SIZE), 
                            (void *)ctx,
                            makeFreeRtosPriority(osPriorityNormal), 
                            &(ctx->handle) );
					// проверяем статус и выходим из цикла если !pdPASS
					if (rc == pdPASS) {
						server_added(px);
					} else {
						status = -1;
						//LWIP_DEBUGF( NET_DEBUG, ("net_srv.c::dispatcher => can't create \"%s\"\r\n", 
            //               (const char *)pcTaskGetName(NULL)) );
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
	poll( void *pargv ) {
/*----------------------------------------------------------------------------*/
	s32_t sock;
	struct sockaddr_in remote;
	u32_t remotelen = sizeof(remote);
	char name[8];
  srv_ctx_t *ctx = (srv_ctx_t *)pargv;
  BaseType_t rc = pdPASS;
  //int rc = pdPASS;
  
  // проверка арг-ов
	if (!ctx) goto exit;
  // debug
  //LWIP_DEBUGF( NET_DEBUG, ("net_srv.c::poll => task \"%s\" created\r\n", 
  //                         (const char *)pcTaskGetName(NULL)) );
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
					ctx->axRmtClt[ig].xData.slSock = sock;
          ctx->axRmtClt[ig].pxFn = ctx->pxFn;
          ctx->axRmtClt[ig].pvDeleted = (task_cb_fn_t)remote_client_deleted_cb;
          ctx->axRmtClt[ig].pvPld = (void *)ctx;
					// даем имя строке
					sprintf(name, "clt%02d", ig);
					// запуск отдельной задачи под каждого нового клиента
					rc = xTaskCreate( net__session,
                            name,
                            (6*configMINIMAL_STACK_SIZE), 
                            &(ctx->axRmtClt[ig]),
                            makeFreeRtosPriority(osPriorityNormal), 
                            &(ctx->axRmtClt[ig].handle) );
          // проверяем статус и выходим из цикла если !pdPASS
					if (rc == pdPASS) {
						remote_client_added(ctx);
					} else {
						//LWIP_DEBUGF( NET_DEBUG, ("net_srv.c::poll => can't create \"%s\"\r\n", 
            //               (const char *)pcTaskGetName(NULL)) );
            LWIP_DEBUGF( NET_DEBUG, ("can't create \"%s\", in '%s' /NET/net_srv.c:%d\r\n", 
              (const char *)pcTaskGetName(NULL), __FUNCTION__, __LINE__) );
					}   
					break;
				}
			}
		}
    // задержка / переключение контекста
    //vTaskDelay(100);
	// ... до тех пор, пока активны флаги MBTCP_OK или MBTCP_NOCLIENT. Иначе в 
	// случае флага MBTCP_LOSECON выйти из цикла.
	} while ( (sock >= 0) /*ctx->bEnable*/ & (rc==pdPASS) );

exit:
  //LWIP_DEBUGF( NET_DEBUG, ("net_srv.c::poll => task \"%s\" deleted\r\n", 
  //                       (const char *)pcTaskGetName(NULL)) );
  LWIP_DEBUGF( NET_DEBUG, ("Server \"%s\" deleted, in '%s' /NET/net_srv.c:%d\r\n", 
    (const char *)pcTaskGetName(NULL), __FUNCTION__, __LINE__) );
  // Закрыть ранее открытый сокет нового подключения, Удаляем запись о 
  // подключении, Удаляем задачу
  if (ctx->slSockServ != -1) {
    lwip_close(ctx->slSockServ);
  }
  if (ctx->pvDeleted) ctx->pvDeleted(ctx->pvPld);
  memset(ctx, 0, sizeof(sess_ctx_t));
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
  //int timeval = 2500;
  //int timeout = 10;
  //int timeval = 20; // = {.tv_sec = 0, .tv_usec = 10000};

	// 1/?. Создаем общий сетевой сокет TCP.
	psrv->slSockServ = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (psrv->slSockServ < 0) {
		return -1; //TODO надо как-то обработать ошибку
	}
  
  //
  //ioctlsocket(psrv->slSockServ, FIONBIO, &on);
  //
  //LWIP_SO_SNDRCVTIMEO_SET(timeval, 20);
  //err = lwip_setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	
  //ioctlsocket(psrv->slSockServ, FIONBIO, &on); //достаточно этой строчки
  /*if (setsockopt(psrv->slSockServ, SOL_SOCKET, SO_RCVTIMEO, &timeval, sizeof(timeval))) {
		goto gtFail;
	}*/
  
	//set master socket to allow multiple connections , this is just a good habit, it will work without this
	if(setsockopt(psrv->slSockServ, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) {
		goto gtFail;
	}
	// KEEP ALIVE
	/*if (setsockopt(psrv->slSockServ, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof on)) {
		goto gtFail;
	}*/
	
	// 2/?. Заполняем структуру со след.полями
	memset(&local, 0, sizeof(local));
	local.sin_family      = AF_INET;
	local.sin_port        = PP_HTONS(psrv->ulPort);
	//local.sin_addr.s_addr = inet_addr(SNTP_LOCAL_IP);
	//local.sin_addr.s_addr = psrv->xNetif.ip_addr.addr; //FIXME
	
	// 3/?. bind to local address
	if (lwip_bind(psrv->slSockServ, (struct sockaddr *)&local, sizeof(local)) < 0) {
		goto gtFail;
	}
		
	// 4/?.
	if (lwip_listen(psrv->slSockServ, TCP_LISTEN_BACKLOG) != 0) {
		goto gtFail;
	}
	
	// 5/?. Выставляем флаг активного физического соединения (RJ45 подключен).
	//ptr->bRunned = true;
	// Инициилизируем верхний уровень 
	//psrv->pvUpperInit(&(psrv->pvUpperPld)); //FIXME

	return 0;

	gtFail:
	// закрывать каждый раз, если успешно открывался
	lwip_close(psrv->slSockServ);
	//ptr->bRunned = false;
	return -1;  //TODO надо как-то обработать ошибку
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
static int
	delete(srv_ctx_t *ctx) {
/*----------------------------------------------------------------------------*/
  // Закрыть ранее открытый сокет нового подключения, Удаляем запись о 
  // подключении, Удаляем задачу
  /*if (ctx->slSock != -1) {
    lwip_close(ctx->slSock);
  }
  if (ctx->pvDeleted) ctx->pvDeleted(ctx->pvPld);
  memset(ctx, 0, sizeof(srv_ctx_t));
  ctx->slSock = -1;
	vTaskDelete(handle);*/
  
  //LWIP_DEBUGF( NET_DEBUG, ("net_srv.c::delete => task \"%s\" deleted\r\n", 
  //           (const char *)pcTaskGetName(ctx->handle)) );
  LWIP_DEBUGF( NET_DEBUG, ("task \"%s\" deleted, in '%s' /NET/net_srv.c:%d\r\n", 
    (const char *)pcTaskGetName(ctx->handle), __FUNCTION__, __LINE__) );
  
  // Закрыть ранее открытый сокет нового подключения, Удаляем запись о 
  // подключении, Удаляем задачу
  if (ctx->slSockServ != -1) {
    lwip_close(ctx->slSockServ);
  }
  //taskYIELD();
  if (ctx->pvDeleted) ctx->pvDeleted(ctx->pvPld);
  vTaskDelete(ctx->handle);
  // выключаем
  //memset(ctx, 0, sizeof(srv_ctx_t));
  ctx->handle = NULL;
  ctx->slSockServ = -1;
  return 0;
}
