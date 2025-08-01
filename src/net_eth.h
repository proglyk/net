/**
	******************************************************************************
  * @file    net_eth.h
  * @author  Ilia Proniashin, PJSC Electrovipryamitel
  * @version V1.0.0
  * @date    26-Jule-2025
  * @brief   Implementation of network layer for STM32 platform
  * @attention The source code is presented as is
  *****************************************************************************/

#ifndef _NET_ETH_H_
#define _NET_ETH_H_

#include "stm32f4xx.h"
#include "lwip/netif.h"
#include "stdbool.h"

/*--Идентификаторы задач отслеживания линка RJ-45 и чтения входящих данных----*/

typedef err_t (*input_ptr_t)(struct pbuf *, void *);

typedef struct {
  input_ptr_t       pscInput;
  ETH_HandleTypeDef xHandle;
} net_eth_t;

// Прототипы публичных (public) функций

net_eth_t *net_eth__inst(void);
s32_t net_eth__init(net_eth_t *);
s32_t net_eth__is_links_up(net_eth_t *, bool *);
void  net_eth__irq(net_eth_t *);
void  net_eth__start(net_eth_t *);
void  net_eth__stop(net_eth_t *);
s32_t net_eth__input(net_eth_t *, void *);
err_t net_eth__output(void *, struct pbuf *, void *);

#endif //_NET_ETH_H_