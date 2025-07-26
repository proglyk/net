/**
	******************************************************************************
  * @file    net_conn.c
  * @author  Ilia Proniashin, PJSC Electrovipryamitel
  * @version V0.31.0
  * @date    26-Jule-2025
  * @brief   Implementation of network layer for STM32 platform
  * @attention The source code is presented as is
  *****************************************************************************/

#include "net_conn.h"
#include "lwip/def.h"
#include "lwip/sockets.h"
#include <string.h>
#include "proj_conf.h"
#include "dbg.h"

// Прототипы локальных (static) функций

static int delete(conn_ctx_t *, TaskHandle_t);

// Публичные (public) функции

/**	----------------------------------------------------------------------------
	* @brief ??? */
void
	net_conn__do(void * argv) {
/*----------------------------------------------------------------------------*/
  s32_t err;
  conn_ctx_t *sess_ctx = (conn_ctx_t*)argv;
  void *user_ctx;
  
  // проверка арг-ов
  if (!sess_ctx) goto exit;
  if ( (!sess_ctx->pxFn->ppvSessInit) | (!sess_ctx->pxFn->pslSessDo) )
    goto exit;
  
  DBG_PRINT( NET_DEBUG, ("Task '%s' created, in '%s' /NET/net_sess.c:%d\r\n", 
    (const char *)pcTaskGetName(NULL), __FUNCTION__, __LINE__) );

  // шаг 1/3. Проводим инициализацию (если таковая требуется)
  user_ctx = sess_ctx->pxFn->ppvSessInit( sess_ctx->pxFn->ppvSessInitCb,
                                          &sess_ctx->xData,
                                          sess_ctx->pxFn->pvUpper );
  if (user_ctx == NULL) goto exit;
	
	// шаг 2/3. Запускаем цикл чтения очередного входящего пакета до тех пор...
	do {
		// Читаем данные и отчитываемся о статусе.
		err = sess_ctx->pxFn->pslSessDo(user_ctx);
	// ... до тех пор, пока активен флаг MBTCP_OK. Иначе в случае флагов 
	// MBTCP_NOCLIENT и MBTCP_LOSECON выйти из цикла.
	} while (err == 0);
	
	// шаг 3/3. Освобождаем ресурсы.
  if ( sess_ctx->pxFn->pvSessDel )
    sess_ctx->pxFn->pvSessDel(sess_ctx->pxFn->pvSessDelCb, user_ctx);
  
exit:
  DBG_PRINT( NET_DEBUG, ("Task '%s' deleted, in '%s' /NET/net_sess.c:%d\r\n", 
    (const char *)pcTaskGetName(NULL), __FUNCTION__, __LINE__) );
	delete(sess_ctx, NULL);
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
int
	net_conn__del(conn_ctx_t *ctx, TaskHandle_t handle) {
/*----------------------------------------------------------------------------*/
  return delete(ctx, handle);
}

// Локальные (static) функции

/**	----------------------------------------------------------------------------
	* @brief ??? */
static int
	delete(conn_ctx_t *ctx, TaskHandle_t handle) {
/*----------------------------------------------------------------------------*/
  // Закрыть ранее открытый сокет нового подключения, Удаляем запись о 
  // подключении, Удаляем задачу
  if (ctx->xData.slSock != -1) {
    lwip_close(ctx->xData.slSock);
  }
  if (ctx->pvDeleted) ctx->pvDeleted(ctx->pvPld);
  memset(ctx, 0, sizeof(conn_ctx_t));
  ctx->xData.slSock = -1;
	vTaskDelete(handle);
  return 0;
}