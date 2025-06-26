#include "net_sess.h"
#include "lwip/def.h"
#include "lwip/sockets.h"
#include <string.h>
#include "config_proj.h"
#include "dbg.h"

static int delete(sess_ctx_t *ctx, TaskHandle_t handle);

//extern bool bShellActive;

void
	net__session(void * argv) {
/*----------------------------------------------------------------------------*/
  s32_t err;
  sess_ctx_t *sess_ctx = (sess_ctx_t*)argv;
  void *user_ctx;
  
  // �������� ���-��
  if (!sess_ctx) goto exit;
  if ( (!sess_ctx->pxFn->ppvSessInit) | (!sess_ctx->pxFn->pslSessDo) )
    goto exit;
  
  //LWIP_DEBUGF( NET_DEBUG, ("Task '%s' created, in '%s' /NET/net_sess.c:%d\r\n", 
  //  (const char *)pcTaskGetName(NULL), __FUNCTION__, __LINE__) );
  DBG_PRINT( NET_DEBUG, ("Task '%s' created, in '%s' /NET/net_sess.c:%d\r\n", 
    (const char *)pcTaskGetName(NULL), __FUNCTION__, __LINE__) );

  // ��� 1/3. �������� ������������� (���� ������� ���������)
  user_ctx = sess_ctx->pxFn->ppvSessInit( sess_ctx->pxFn->ppvSessInitCb,
                                          &sess_ctx->xData,
                                          sess_ctx->pxFn->pvUpper );
  if (user_ctx == NULL) goto exit;
	
	// ��� 2/3. ��������� ���� ������ ���������� ��������� ������ �� ��� ���...
	do {
		// ������ ������ � ������������ � �������.
		err = sess_ctx->pxFn->pslSessDo(user_ctx);
	// ... �� ��� ���, ���� ������� ���� MBTCP_OK. ����� � ������ ������ 
	// MBTCP_NOCLIENT � MBTCP_LOSECON ����� �� �����.
	} while (err == 0);
	
	// ��� 3/3. ����������� �������.
  if ( sess_ctx->pxFn->pvSessDel )
    sess_ctx->pxFn->pvSessDel(sess_ctx->pxFn->pvSessDelCb, user_ctx);
  
exit:
  //LWIP_DEBUGF( NET_DEBUG, ("Task '%s' deleted, in '%s' /NET/net_sess.c:%d\r\n", 
  //  (const char *)pcTaskGetName(NULL), __FUNCTION__, __LINE__) );
  DBG_PRINT( NET_DEBUG, ("Task '%s' deleted, in '%s' /NET/net_sess.c:%d\r\n", 
    (const char *)pcTaskGetName(NULL), __FUNCTION__, __LINE__) );
	delete(sess_ctx, NULL);
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
int
	net_sess__delete(sess_ctx_t *ctx, TaskHandle_t handle) {
/*----------------------------------------------------------------------------*/
  return delete(ctx, handle);
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
static int
	delete(sess_ctx_t *ctx, TaskHandle_t handle) {
/*----------------------------------------------------------------------------*/
  // ������� ����� �������� ����� ������ �����������, ������� ������ � 
  // �����������, ������� ������
  if (ctx->xData.slSock != -1) {
    lwip_close(ctx->xData.slSock);
  }
  if (ctx->pvDeleted) ctx->pvDeleted(ctx->pvPld);
  memset(ctx, 0, sizeof(sess_ctx_t));
  ctx->xData.slSock = -1;
	vTaskDelete(handle);
  return 0;
}