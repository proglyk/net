#ifndef _NET_CLT_H_
#define _NET_CLT_H_

#include "config_proj.h"
#include "net_if.h"
#include "net_sess.h"
#include "lwip/sockets.h"
#include <stdbool.h>

typedef struct {  
  // ����������
  struct sockaddr_in remote;
  u8_t acName[16];
  s32_t ulSockType;
  bool bEnable;
  // ������������
  u32_t ulId;
  u32_t ulPort; //remote
  ip_addr_t xIpRmt;
  //ip_addr_t xIpAddr; //my ip
  // ���������� �� ����������� ���� �������.
  // ���� ���� - ���� ������ - ���� ����������� (���� ��������� ������)
  //TaskHandle_t 	pvRmtSrvHndl;
  sess_ctx_t    xRmtSrv;
  //int vars_clt;
  
  net_if_fn_t *pxFn;
} clt_ctx_t;

typedef struct {
  // �������������� �������� tcp
  bool bRunned;
  TaskHandle_t 	pvCltHndl;
  bool bNetifIsUp;
  clt_ctx_t axClients[CLT_NUM_MAX];
} net_clt_t;

// ��������� ��������� (public) �������

int	net_clt__task_create(net_clt_t *);

#endif //_NET_CLT_H_