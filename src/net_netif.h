/**
	******************************************************************************
  * @file    net_netif.h
  * @author  Ilia Proniashin, PJSC Electrovipryamitel
  * @version V0.31.0
  * @date    26-Jule-2025
  * @brief   Implementation of network layer for STM32 platform
  * @attention The source code is presented as is
  *****************************************************************************/

#ifndef _NET_LINK_H_
#define _NET_LINK_H_

#include "net_eth.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define MAX_DHCP_TRIES  6

typedef enum {
	IPSTATIC=0,
	IPDYNAMIC
} IpType;

typedef enum {
	DHCP_OFF=0,
	DHCP_START,
	DHCP_WAIT_ADDRESS,
	DHCP_ADDRESS_ASSIGNED,
	DHCP_TIMEOUT,
	DHCP_LINK_DOWN,
	DHCP_ERROR
} DhcpState;

typedef struct {
  IpType					eType;
//  struct {
//    u8_t 					ipv4[4];
//    u8_t 					mask[4];
//    u8_t 					gw[4];
//  } xStatic;
  struct {
    DhcpState			eState;
  } xDynamic;
} ip_t;

typedef struct {
  struct dhcp *			pxData;
  TaskHandle_t 			pvHandle;
  SemaphoreHandle_t	pvSmphrAssign;
} clnt_dchp_t;

typedef void (*netif_is_up_ptr_t)(bool);

typedef struct {
  TaskHandle_t 			pvTaskLink;
  netif_is_up_ptr_t pvNetifCb;
  // Способ присвоения IPv4 адреса устройству
  ip_t xIp;
  SemaphoreHandle_t	pvSmphrInput;
  TaskHandle_t 			pvTaskInput;
  struct netif xNetif;
  // клиент dhcp
  clnt_dchp_t   xDhcp;
  // аппаратный уровень
  //net_eth_t       xEthif;
} net_netif_t;

net_netif_t *net_netif__inst(void);
void         net_netif__task_create( net_netif_t * );
s32_t        net_netif__init( net_netif_t *, netif_is_up_ptr_t );
void         net_netif__sig_input( net_netif_t *px );
ip_addr_t   *net_netif__get_addr(void);

#endif //_NET_LINK_H_