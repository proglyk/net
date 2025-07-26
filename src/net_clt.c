#include "net_clt.h"
#include "net_netif.h"
#include "stdlib.h"
#include "string.h"
#include "userlib.h"
#include "lwip/def.h"

// Переменные, константы публичные (public)

extern int errno;

// Прототипы локальных (static) функций

static s32_t get_clients_number(net_clt_t *);
static void  dispatcher(void *);
static int   try_connect(clt_ctx_t *, int *);

// Публичные (public) функции

/**	----------------------------------------------------------------------------
	* @brief ??? */
int
	net_clt__task_create(net_clt_t *px) {
/*----------------------------------------------------------------------------*/
	return xTaskCreate( dispatcher, 
                      "clt_disp", 
                      (3*configMINIMAL_STACK_SIZE), 
                      (void*)px, //FIXME
                      uxGetPriority(PRIO_NORM),
                      &(px->pvCltHndl) );
}

// Локальные (static) функции

/**	----------------------------------------------------------------------------
	* @brief ???
	* @retval error: Статус выполнения функции. */
static s32_t
  get_clients_number(net_clt_t *px) {
/*----------------------------------------------------------------------------*/
	s32_t num = 0;
  if (!px) return -1;
  
  for (uint8_t i = 0; i < CLT_NUM_MAX; i++) {
    if (strcmp( (const char *)px->axClients[i].acName, "" ))
      num += 1;
	}
	return num;
}

/**	----------------------------------------------------------------------------
	* @brief ??? */  
static void
  dispatcher(void *pv) {
/*----------------------------------------------------------------------------*/
  BaseType_t rc;
  int status = 0;
  s32_t err;
  clt_ctx_t *ctx;
  net_clt_t *px = (net_clt_t *)pv;
  int sock = -1;
   
  int size = get_clients_number(px);
  if (size < 0) return;
  
  do {
    for (int i=0; i<size; i++) {
      ctx = (clt_ctx_t *)(px->axClients + i);
      if ( (px->bNetifIsUp)&(!ctx->xRmtSrv.handle)&(ctx->bEnable) ) {
        err = try_connect(ctx, &sock);
        if (err >= 0) {
          // Индикация в uart
          LWIP_DEBUGF( NET_DEBUG, ("Client \"%s\" connected, in '%s' /NET/net_clt.c:%d\r\n", 
              (const char *)pcTaskGetName(NULL), __FUNCTION__, __LINE__) );
          // запуск отдельной задачи под каждого нового клиента
          ctx->xRmtSrv.xData.ulId = ctx->ulId;
          ctx->xRmtSrv.xData.slSock = sock;
          ctx->xRmtSrv.xData.xAddrRmt.addr = ctx->xIpRmt.addr;
          ctx->xRmtSrv.xData.ulPortRmt = ctx->ulPort;
          ctx->xRmtSrv.pxFn = ctx->pxFn;
          rc = xTaskCreate( net_conn__do, 
                            (const char *)ctx->acName,
                            (4*configMINIMAL_STACK_SIZE), 
                            (void *)&(ctx->xRmtSrv),
                            uxGetPriority(PRIO_NORM),
                            &(ctx->xRmtSrv.handle) );
          if (rc != pdPASS ) {
            status = -1;
            LWIP_DEBUGF( NET_DEBUG, ("%s can't create sess task, in '%s' /NET/net_clt.c:%d\r\n", 
              ctx->acName, __FUNCTION__, __LINE__) );
            break;
          }
        } else {
          LWIP_DEBUGF( NET_DEBUG, ("%s can't connect to %s, in %s /NET/net_clt.c:%d\r\n", 
              ctx->acName, ctx->pcIpRmt, __FUNCTION__, __LINE__) );
        }
      }
    }
    // задержка перед новой попыткой подключиться
    vTaskDelay(10000);
  } while (status == 0);
  
  return;
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
static int
	try_connect(clt_ctx_t *ctx, int *psock) {
/*----------------------------------------------------------------------------*/
  int err;
  int sock;
  struct sockaddr_in local;
  
  sock = lwip_socket( AF_INET, ctx->ulSockType,
           (ctx->ulSockType == SOCK_STREAM) ? (IPPROTO_TCP) : (IPPROTO_UDP) );
  
  if (sock < 0) {
    *psock = -1;
    return -1;
  }  
  
  // ХЗ зачем
	memset(&local, 0, sizeof(local));
	local.sin_family      = AF_INET;
	local.sin_port        = PP_HTONS(50000 + ctx->ulId);
  local.sin_addr.s_addr = net_netif__get_addr()->addr;
  if (lwip_bind(sock, (struct sockaddr *)&local, sizeof(local)) < 0) 
    goto fail;

  if (ctx->ulSockType == SOCK_STREAM) {
    // 4.
    memset(&ctx->remote, 0, sizeof(struct sockaddr_in));
    ctx->remote.sin_family = AF_INET;
    ctx->remote.sin_port = htons(ctx->ulPort);
    ctx->remote.sin_addr.s_addr = ctx->xIpRmt.addr;
    err = lwip_connect( sock, (struct sockaddr *)&ctx->remote, 
                        sizeof(ctx->remote) );
    if (err < 0) goto fail;
  }
    
  *psock = sock;
  return 0;
  
  fail:
  // сбросить errno чтобы не мешало другим задачам
  if (errno != 0) errno = 0;
	// закрывать каждый раз, если успешно открывался
	lwip_close(sock); *psock = -1;
  return -1;
}
