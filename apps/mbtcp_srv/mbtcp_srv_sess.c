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

// привязки
/*net_if_fn_t xFnSrvMbtcp = {
  .pvUpperInit = NULL,
  .pvUpper = NULL,
  .ppvSessInit = init_session,
  .pslSessDo = do_session,
  .pvSessDel = del_sess,
};*/
// привязки
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
	* @param sock: Сокет подключения
	* @param pvCtx: ???
	* @param pvUpper: ???
	* @retval Статус выполнения */
static void *
  init_session(sess_init_cb_ptr_t unused, net_if_data_t *pdata, void* pvUpper) {
/*----------------------------------------------------------------------------*/
  int timeval = 10000;
  int on=1;
  ctx_t *ctx = malloc(sizeof(ctx_t));
  // проверка арг-ов
  if (!ctx || !pdata) return NULL;
  if ((pdata->slSock < 0)) return NULL;
  
  // копируем данные
  ctx->slSock = pdata->slSock;
  if (setsockopt(ctx->slSock, SOL_SOCKET, SO_RCVTIMEO, &timeval, sizeof(timeval))) {
    return NULL;
  }
  if (setsockopt(ctx->slSock, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof on)) {
		return NULL;
	}
  
  // инициализация верхнего уровня
  mb_Init(&(ctx->xMbcb));
  
  //LWIP_DEBUGF( NET_DEBUG, ("Sess 's=%d' created, in '%s' /MBTCP/mbtcp_srv_sess.c:%d\r\n", 
  //  ctx->slSock, __FUNCTION__, __LINE__) );
  DBG_PRINT( NET_DEBUG, ("Sess 's=%d' created, in '%s' /MBTCP/mbtcp_srv_sess.c:%d\r\n", 
    ctx->slSock, __FUNCTION__, __LINE__) );
  
  // возвращаем указат для дальнейшего исп.-ния
  return (void *)ctx;
}

/**	----------------------------------------------------------------------------
	* @brief 
	* @param argv: ???
	* @retval Статус выполнения */
static signed long
  do_session(void* argv) {
/*----------------------------------------------------------------------------*/
  //sess_ctx_t *ctx = NULL;
  mbtcp_err_t result;
  u32_t recvd, written;
  u8_t pcHeader[MBTCP_HEADSIZE];
  ctx_t* ctx = (ctx_t *)argv;
  s32_t rc = 0;
  
  // проверка арг-ов
  if (!ctx) return -1;
  
	// Ожидаем приема новых байт заголовка сообщения Modbus TCP. Блокируемся.
	result = receive(ctx->slSock, pcHeader, sizeof(pcHeader), &recvd);
	if ( result != MBTCP_OK ) goto err_rcv;
	
	// Нормальный случай
	// Копируем поля TransID, ProtID и Packet lenght.
	ctx->usTransID = PBTOS(pcHeader+0);
	ctx->usProtID = PBTOS(pcHeader+2);
	ctx->usLenPDU = PBTOS(pcHeader+4);
	
	// Далее принимает остальные байты пакета. Блокируемся, но совсем ненадолго.
	if ((result = receive(ctx->slSock, ctx->xMbcb.xData.xIn.pcBuf, sizeof(ctx->
    xMbcb.xData.xIn.pcBuf),	&recvd)) != MBTCP_OK)	goto err_rcv;
	// Проверка соответсвия длины сообщения в заголовке и его реального размера
	if (recvd != ctx->usLenPDU)	return 0;
	// Копируем число прочтенных байт в поле размера входного буффера
	ctx->xMbcb.xData.xIn.usSize = (u16_t)recvd;
	
	// Вызываем функцию Modbus TCP
	mbtcp_srv_check(&(ctx->xMbcb)); // TODO
	// Сбрасываем счетчик наличия обмена
	ctx->usClientConnect += 1;
	
	// Формирую ответ TCP
	memmove(ctx->xMbcb.xData.xOut.pcBuf + MBTCP_HEADSIZE, ctx->xMbcb.xData.
		xOut.pcBuf, ctx->xMbcb.xData.xOut.usSize);
	ctx->xMbcb.xData.xOut.usSize += MBTCP_HEADSIZE;
	// Копируем
	STOPB((ctx->xMbcb.xData.xOut.pcBuf+0), ctx->usTransID);
	STOPB((ctx->xMbcb.xData.xOut.pcBuf+2), ctx->usProtID);
	ctx->xMbcb.xData.xOut.pcBuf[4] = 0;
	ctx->xMbcb.xData.xOut.pcBuf[5] = (u8_t)(ctx->xMbcb.xData.xOut.usSize - MBTCP_HEADSIZE);
	// Отправляем
	// Возврат состояния.
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
	* @brief Функция чтения буфера входных данных LwIP с блокировкой
	* @param sock: Сокет подключения.
	* @param pBuf: Указатель на массив куда считывать.
	* @param len: Запрошенная длина массива байт для чтения.
	* @param received: Число успешно прочитанных из общего запрошенного числа байт.
	* @retval error: Статус выполнения функции. */
static mbtcp_err_t
	receive(s32_t sock, u8_t* pBuf, u32_t len, u32_t* received) {
/*----------------------------------------------------------------------------*/
	s32_t bytes;
	
	// Ожидаем приема нового пакета. Блокируемся.
	bytes = lwip_recvfrom(sock, pBuf, len, 0, NULL, NULL);
	// Условия проверки статуса выполнения lwip_recvfrom
	// Случай вынимания разъема RJ45. Тогда приходит bytes=-1.
	if (bytes < 0)
		return MBTCP_LOSECON;
	// Случай отключения клиента. Тогда приходит bytes=0.
	if (bytes == 0)
		return MBTCP_NOCLIENT;

	// Копируем считанную длину
	*received = bytes;
	
	return MBTCP_OK;
}


/**	----------------------------------------------------------------------------
	* @brief Функция записи в буфер выходных данных LwIP
	* @param sock: Сокет подключения.
	* @param pBuf: Указатель на массив откуда брать для записи.
	* @param len: Размер массива байт.
	* @param written: Число успешно записанных из общего числа байт.
	* @retval error: Статус выполнения функции. */
static mbtcp_err_t
	transmit(s32_t sock, u8_t* pBuf, u32_t len, u32_t* written) {
/*----------------------------------------------------------------------------*/
	s32_t bytes;
	
	// Ожидаем приема нового пакета. Блокируемся.
	bytes = lwip_send(sock, pBuf, len, 0);
	// Условия проверки статуса выполнения lwip_recvfrom
	// Случай вынимания разъема RJ45. Тогда приходит bytes=-1.
	if (bytes < 0)
		return MBTCP_LOSECON;
	// Случай отключения клиента. Тогда приходит bytes=0.
	if (bytes == 0)
		return MBTCP_NOCLIENT;

	// Копируем считанную длину
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
