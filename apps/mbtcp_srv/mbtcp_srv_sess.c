#include "mbtcp_srv_sess.h"
#include "net_if.h"
#include "string.h"
#include "lwip/sockets.h"
#include "mbtcp_srv.h"
#include "config_proj.h"
#include "dbg.h"

static void *init_session(sess_init_cb_ptr_t, net_if_data_t *, void*);
static signed long do_session(void*);
static void del_sess(sess_del_cb_ptr_t, void*);
static mbtcp_err_t receive(s32_t, u8_t*, u32_t, u32_t*);
static mbtcp_err_t transmit(s32_t, u8_t*, u32_t, u32_t*);
static const char *get_err_str(s16_t);

// ��������
/*net_if_fn_t xFnSrvMbtcp = {
  .pvUpperInit = NULL,
  .pvUpper = NULL,
  .ppvSessInit = init_session,
  .pslSessDo = do_session,
  .pvSessDel = del_sess,
};*/
// ��������
net_if_fn_t xFnSrvMbtcp = {
  .pvUpperInit =   (upper_init_ptr_t)NULL,
  .pvUpper =       (void *)NULL,
  .ppvSessInit =   (sess_init_ptr_t)init_session,
  .ppvSessInitCb = (sess_init_cb_ptr_t)NULL,
  .pslSessDo =     (sess_do_ptr_t)do_session,
  .pvSessDel =     (sess_del_ptr_t)del_sess,
  .pvSessDelCb =   (sess_del_cb_ptr_t)NULL,
};

/**	----------------------------------------------------------------------------
	* @brief ???
	* @param sock: ����� �����������
	* @param pvCtx: ???
	* @param pvUpper: ???
	* @retval ������ ���������� */
static void *
  init_session(sess_init_cb_ptr_t unused, net_if_data_t *pdata, void* pvUpper) {
/*----------------------------------------------------------------------------*/
  int timeval = 10000;
  int on=1;
  ctx_t *ctx = malloc(sizeof(ctx_t));
  // �������� ���-��
  if (!ctx || !pdata) return NULL;
  if ((pdata->slSock < 0)) return NULL;
  
  // �������� ������
  ctx->slSock = pdata->slSock;
  if (setsockopt(ctx->slSock, SOL_SOCKET, SO_RCVTIMEO, &timeval, sizeof(timeval))) {
    return NULL;
  }
  if (setsockopt(ctx->slSock, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof on)) {
		return NULL;
	}
  
  // ������������� �������� ������
  mb_Init(&(ctx->xMbcb));
  
  //LWIP_DEBUGF( NET_DEBUG, ("Sess 's=%d' created, in '%s' /MBTCP/mbtcp_srv_sess.c:%d\r\n", 
  //  ctx->slSock, __FUNCTION__, __LINE__) );
  DBG_PRINT( NET_DEBUG, ("Sess 's=%d' created, in '%s' /MBTCP/mbtcp_srv_sess.c:%d\r\n", 
    ctx->slSock, __FUNCTION__, __LINE__) );
  
  // ���������� ������ ��� ����������� ���.-���
  return (void *)ctx;
}

/**	----------------------------------------------------------------------------
	* @brief 
	* @param argv: ???
	* @retval ������ ���������� */
static signed long
  do_session(void* argv) {
/*----------------------------------------------------------------------------*/
  //sess_ctx_t *ctx = NULL;
  mbtcp_err_t result;
  u32_t recvd, written;
  u8_t pcHeader[MBTCP_HEADSIZE];
  ctx_t* ctx = (ctx_t *)argv;
  s32_t rc = 0;
  
  // �������� ���-��
  if (!ctx) return -1;
  
	// ������� ������ ����� ���� ��������� ��������� Modbus TCP. �����������.
	result = receive(ctx->slSock, pcHeader, sizeof(pcHeader), &recvd);
	if ( result != MBTCP_OK ) goto err_rcv;
	
	// ���������� ������
	// �������� ���� TransID, ProtID � Packet lenght.
	ctx->usTransID = PBTOS(pcHeader+0);
	ctx->usProtID = PBTOS(pcHeader+2);
	ctx->usLenPDU = PBTOS(pcHeader+4);
	
	// ����� ��������� ��������� ����� ������. �����������, �� ������ ���������.
	if ((result = receive(ctx->slSock, ctx->xMbcb.xData.xIn.pcBuf, sizeof(ctx->
    xMbcb.xData.xIn.pcBuf),	&recvd)) != MBTCP_OK)	goto err_rcv;
	// �������� ����������� ����� ��������� � ��������� � ��� ��������� �������
	if (recvd != ctx->usLenPDU)	return 0;
	// �������� ����� ���������� ���� � ���� ������� �������� �������
	ctx->xMbcb.xData.xIn.usSize = (u16_t)recvd;
	
	// �������� ������� Modbus TCP
	mbtcp_srv_check(&(ctx->xMbcb)); // TODO
	// ���������� ������� ������� ������
	ctx->usClientConnect += 1;
	
	// �������� ����� TCP
	memmove(ctx->xMbcb.xData.xOut.pcBuf + MBTCP_HEADSIZE, ctx->xMbcb.xData.
		xOut.pcBuf, ctx->xMbcb.xData.xOut.usSize);
	ctx->xMbcb.xData.xOut.usSize += MBTCP_HEADSIZE;
	// ��������
	STOPB((ctx->xMbcb.xData.xOut.pcBuf+0), ctx->usTransID);
	STOPB((ctx->xMbcb.xData.xOut.pcBuf+2), ctx->usProtID);
	ctx->xMbcb.xData.xOut.pcBuf[4] = 0;
	ctx->xMbcb.xData.xOut.pcBuf[5] = (u8_t)(ctx->xMbcb.xData.xOut.usSize - MBTCP_HEADSIZE);
	// ����������
	// ������� ���������.
  rc = transmit(ctx->slSock, ctx->xMbcb.xData.xOut.pcBuf, ctx->xMbcb.xData.
		xOut.usSize, &written);
  if (rc != MBTCP_OK) {
    //LWIP_DEBUGF( NET_DEBUG, ("Can't transmit msg ('%s'), in '%s' /MBTCP/"     \
    //  "mbtcp_srv_sess.c:%d\r\n", get_err_str(result), __FUNCTION__, __LINE__) );
    DBG_PRINT( NET_DEBUG, ("Can't transmit msg ('%s'), in '%s' /MBTCP/"  \
      "mbtcp_srv_sess.c:%d\r\n", get_err_str(result), __FUNCTION__, __LINE__) );
  }
	return rc;
  
  err_rcv:
  //LWIP_DEBUGF( NET_DEBUG, ("Break waiting for recv ('%s'), in '%s' /MBTCP/"      \
  //  "mbtcp_srv_sess.c:%d\r\n", get_err_str(result), __FUNCTION__, __LINE__) );
  DBG_PRINT( NET_DEBUG, ("Break waiting for recv ('%s'), in '%s' /MBTCP/" \
    "mbtcp_srv_sess.c:%d\r\n", get_err_str(result), __FUNCTION__, __LINE__) );
  return -1;
}

/**	----------------------------------------------------------------------------
	* @brief ������� ������ ������ ������� ������ LwIP � �����������
	* @param sock: ����� �����������.
	* @param pBuf: ��������� �� ������ ���� ���������.
	* @param len: ����������� ����� ������� ���� ��� ������.
	* @param received: ����� ������� ����������� �� ������ ������������ ����� ����.
	* @retval error: ������ ���������� �������. */
static mbtcp_err_t
	receive(s32_t sock, u8_t* pBuf, u32_t len, u32_t* received) {
/*----------------------------------------------------------------------------*/
	s32_t bytes;
	
	// ������� ������ ������ ������. �����������.
	bytes = lwip_recvfrom(sock, pBuf, len, 0, NULL, NULL);
	// ������� �������� ������� ���������� lwip_recvfrom
	// ������ ��������� ������� RJ45. ����� �������� bytes=-1.
	if (bytes < 0)
		return MBTCP_LOSECON;
	// ������ ���������� �������. ����� �������� bytes=0.
	if (bytes == 0)
		return MBTCP_NOCLIENT;

	// �������� ��������� �����
	*received = bytes;
	
	return MBTCP_OK;
}


/**	----------------------------------------------------------------------------
	* @brief ������� ������ � ����� �������� ������ LwIP
	* @param sock: ����� �����������.
	* @param pBuf: ��������� �� ������ ������ ����� ��� ������.
	* @param len: ������ ������� ����.
	* @param written: ����� ������� ���������� �� ������ ����� ����.
	* @retval error: ������ ���������� �������. */
static mbtcp_err_t
	transmit(s32_t sock, u8_t* pBuf, u32_t len, u32_t* written) {
/*----------------------------------------------------------------------------*/
	s32_t bytes;
	
	// ������� ������ ������ ������. �����������.
	bytes = lwip_send(sock, pBuf, len, 0);
	// ������� �������� ������� ���������� lwip_recvfrom
	// ������ ��������� ������� RJ45. ����� �������� bytes=-1.
	if (bytes < 0)
		return MBTCP_LOSECON;
	// ������ ���������� �������. ����� �������� bytes=0.
	if (bytes == 0)
		return MBTCP_NOCLIENT;

	// �������� ��������� �����
	*written = bytes;
	
	return MBTCP_OK;
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
static void
  del_sess(sess_del_cb_ptr_t unused, void *argv) {
/*----------------------------------------------------------------------------*/
  ctx_t* ctx = (ctx_t *)argv;
  if (!ctx) return;
  
  DBG_PRINT( NET_DEBUG, ("Sess 's=%d' closed, in '%s' /MBTCP/mbtcp_srv_sess.c:%d\r\n", 
    ctx->slSock, __FUNCTION__, __LINE__) );
  
  free(ctx); 
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
static const char *
  get_err_str(s16_t code) {
/*----------------------------------------------------------------------------*/
  u8_t *str = NULL;
  static const char *pcUnk = "UNK";
  static const char *pcOk = "OK";
  static const char *pcNoclient = "NOCLIENT";
  static const char *pcLosecon = "LOSECON";
  
  switch (code) {
    case  MBTCP_UNK:      str = (u8_t *)pcUnk; break;
    case  MBTCP_OK:       str = (u8_t *)pcOk; break;
    case  MBTCP_NOCLIENT: str = (u8_t *)pcNoclient; break;
    case  MBTCP_LOSECON:  str = (u8_t *)pcLosecon; break;
    default: str = (u8_t *)"ERR"; break;
  }
  return (const char *)str;
}
