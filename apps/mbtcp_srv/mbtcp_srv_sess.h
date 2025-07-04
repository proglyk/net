#ifndef _MBTCP_SRV_SESS_H_
#define _MBTCP_SRV_SESS_H_

#include "userint.h"
#include "mbtcp_srv.h"

// ������ ��������� ��������� Modbus TCP
#define MBTCP_HEADSIZE							6

typedef enum {
	MBTCP_UNK				=-1,
	MBTCP_OK 				= 0,
	MBTCP_NOCLIENT	= 1,
	MBTCP_LOSECON		= 2
} mbtcp_err_t;

typedef struct {
  s32_t slSock;
	// ��� ���������
//	const mb_type_t eCommProt;
	// ����� ����� ���������� � ���� Modbus
	const u16_t usSlaveID;
	// ID ����������
	u16_t usTransID;
	// ID ���������
	u16_t usProtID;
	// ����� ����� PDU
	u16_t usLenPDU;
// ��������� �� ������ ������ � ������ -----------------------------------------
  mbtcp_srv_t xMbcb;
// ������ ----------------------------------------------------------------------
//	mbtcp_err_t eStatus;
//	bool bRunned;
// ���� ������� ������ �� TCP --------------------------------------------------
	u16_t usClientConnect;
} ctx_t;


#endif //_MBTCP_SRV_SESS_H_