#ifndef _MBTCP_SRV_SESS_H_
#define _MBTCP_SRV_SESS_H_

#include "userint.h"
#include "mbtcp_srv.h"

// Размер заголовка сообщения Modbus TCP
#define MBTCP_HEADSIZE							6

typedef enum {
	MBTCP_UNK				=-1,
	MBTCP_OK 				= 0,
	MBTCP_NOCLIENT	= 1,
	MBTCP_LOSECON		= 2
} mbtcp_err_t;

typedef struct {
  s32_t slSock;
	// Тип протокола
//	const mb_type_t eCommProt;
	// Адрес этого устройства в сети Modbus
	const u16_t usSlaveID;
	// ID транзакции
	u16_t usTransID;
	// ID протокола
	u16_t usProtID;
	// Длина блока PDU
	u16_t usLenPDU;
// Указатель на буферы чтения и записи -----------------------------------------
  mbtcp_srv_t xMbcb;
// Статус ----------------------------------------------------------------------
//	mbtcp_err_t eStatus;
//	bool bRunned;
// Флаг наличия обмена по TCP --------------------------------------------------
	u16_t usClientConnect;
} ctx_t;


#endif //_MBTCP_SRV_SESS_H_