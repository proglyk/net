#include "net_eth.h"
#include "lwip/err.h"
#include "string.h"
#include "proj_conf.h"
//#include "net_user.h"

// Прототипы локальных (static) функций

static struct pbuf *read_bytes(ETH_HandleTypeDef *);

// Константы публичные (public)

const uint8_t macaddress[6] = MAC_ADDR; // Массив значений сетевого MAC адреса

// Константы локальные (static)

static const u32_t addr[PHY_MAX_LEN] = PHY_ADDR; // адрес микросхемы
//static const int addr = PHY_ADDR; // адрес микросхемы

// Переменные локальные (static)

static net_eth_t xEth; // инстанс нижнего 'Eth' уровня
// Массивы буфферов DMA
#pragma data_alignment=4 
__ALIGN_BEGIN __attribute__((__section__(".lwip"))) static ETH_DMADescTypeDef
  DMARxDscrTab[ETH_RXBUFNB] __ALIGN_END;
#pragma data_alignment=4 
__ALIGN_BEGIN __attribute__((__section__(".lwip"))) static ETH_DMADescTypeDef
  DMATxDscrTab[ETH_TXBUFNB] __ALIGN_END;
// Массив буфферов для входящих сообщений
#pragma data_alignment=4 
__ALIGN_BEGIN __attribute__((__section__(".lwip"))) static uint8_t
  Rx_Buff[ETH_RXBUFNB][1524] __ALIGN_END;
#pragma data_alignment=4 
__ALIGN_BEGIN __attribute__((__section__(".lwip"))) static uint8_t
  Tx_Buff[ETH_TXBUFNB][1524] __ALIGN_END;

// Публичные (public) функции

/**	----------------------------------------------------------------------------
	* @brief ??? */
net_eth_t *
	net_eth__inst(void) {
/*----------------------------------------------------------------------------*/
	return &xEth;
}

/**	----------------------------------------------------------------------------
	* @brief  ??
	* @param  ptr: ??? */
s32_t
  net_eth__init(net_eth_t *ptr) {
/*----------------------------------------------------------------------------*/
  ETH_HandleTypeDef *phandle;
  static HAL_StatusTypeDef rc;

  if (!ptr) return -1;
  phandle = (ETH_HandleTypeDef *)&(ptr->xHandle);
	phandle->Instance = ETH;
	phandle->Init.MACAddr = (uint8_t *)macaddress;
#if (defined CLT_IS_PI396IZM1)
	phandle->Init.AutoNegotiation = ETH_AUTONEGOTIATION_DISABLE;
#else
	phandle->Init.AutoNegotiation = ETH_AUTONEGOTIATION_ENABLE;
#endif
  phandle->Init.Speed = ETH_SPEED_100M;
	phandle->Init.DuplexMode = ETH_MODE_FULLDUPLEX;
	phandle->Init.MediaInterface = ETH_MEDIA_INTERFACE_RMII;
	phandle->Init.RxMode = ETH_RXINTERRUPT_MODE;
	phandle->Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
#if (defined CLT_IS_PI396IZM1)
  phandle->Init.PhyAddr.Lenght = 2;
	phandle->Init.PhyAddr.aus[0] = (u16_t)addr[0];
  phandle->Init.PhyAddr.aus[1] = (u16_t)addr[1];
  LWIP_DEBUGF( NET_DEBUG, ("PHY1 addr=%01d, PHY2 addr=%01d, in '%s' /NET/net_eth.c:%d\r\n", 
      addr[0], addr[1], __FUNCTION__, __LINE__) );
#else
  phandle->Init.PhyAddr.Lenght = 1;
  phandle->Init.PhyAddr.aus[0] = (u16_t)addr[0];
  LWIP_DEBUGF( NET_DEBUG, ("PHY1 addr=%01d, in '%s' /NET/net_eth.c:%d\r\n", 
      addr[0], __FUNCTION__, __LINE__) );
#endif
	rc = HAL_ETH_Init(phandle);
  // внимание! Случай ошибки далее не обрабатывается! Важно чтобы было OK
  if (rc == HAL_OK) {
    LWIP_DEBUGF( NET_DEBUG, ("PHY init...OK, in '%s' /NET/net_eth.c:%d\r\n", 
      __FUNCTION__, __LINE__) );
  } else {
    LWIP_DEBUGF( NET_DEBUG, ("PHY init...ERROR, in '%s' /NET/net_eth.c:%d\r\n", 
      __FUNCTION__, __LINE__) );
  }
  // Initialize Tx,Rx Descriptors list: Chain Mode
	HAL_ETH_DMATxDescListInit(phandle, DMATxDscrTab, &Tx_Buff[0][0], 
		ETH_TXBUFNB);
	HAL_ETH_DMARxDescListInit(phandle, DMARxDscrTab, &Rx_Buff[0][0], 
		ETH_RXBUFNB);
  
  return rc == HAL_OK ? 0 : -1;
}

/**	----------------------------------------------------------------------------
	* @brief  Чтение статуса регистра BSR n-ного кол-ва интерфейсов PHY
	* @param  ptr: ??? */
s32_t
  net_eth__is_links_up(net_eth_t *px, bool *sta) {
/*----------------------------------------------------------------------------*/
  u32_t reg[PHY_MAX_LEN];
  bool sta_local = false;
  
  for (int i = 0; i < px->xHandle.Init.PhyAddr.Lenght; i++) {
    if ( HAL_ETH_ReadPHYRegister(&(px->xHandle),
                                 px->xHandle.Init.PhyAddr.aus[i],
                                 PHY_BSR,
                                 &reg[i]
                                ) != HAL_OK )  { return -1; }
  }
#if   (defined CLT_IS_PI396IZM1)
  sta_local |= ((reg[0] != 0xffff) && (reg[0] & LAN8710_BSR_LINK_STATUS));
  sta_local |= ((reg[1] != 0xffff) && (reg[1] & LAN8710_BSR_LINK_STATUS));
#else
  sta_local = ((reg[0] != 0xffff) && (reg[0] & LAN8710_BSR_LINK_STATUS));
#endif
  *sta = sta_local;
  return 0;
}

/**	----------------------------------------------------------------------------
	* @brief  ??
	* @param  ptr: ??? */
void
  net_eth__irq(net_eth_t *ptr) {
/*----------------------------------------------------------------------------*/
  HAL_ETH_IRQHandler(&(ptr->xHandle));
}

/**	----------------------------------------------------------------------------
	* @brief  ??
	* @param  ptr: ??? */
void
  net_eth__start(net_eth_t *ptr) {
/*----------------------------------------------------------------------------*/
  HAL_ETH_Start(&(ptr->xHandle));
}

/**	----------------------------------------------------------------------------
	* @brief  ??
	* @param  ptr: ??? */
void
  net_eth__stop(net_eth_t *ptr) {
/*----------------------------------------------------------------------------*/
  HAL_ETH_Stop(&(ptr->xHandle));
}

/**	----------------------------------------------------------------------------
	* @brief  Функция-обработчик входных пакетов в ответ на сигнал семафора
	* @param  argv: Указатель на всё
	* @retval none: Нет */
s32_t
  net_eth__input(net_eth_t *ptr, void * argv) {
/*----------------------------------------------------------------------------*/
struct pbuf *pbuf;

// проверка
if ((!ptr)) return -1;
if ((!ptr->pscInput)) return -1;

// бегунок по принятым сообщения pbuf
do {
  pbuf = read_bytes(&(ptr->xHandle)); //FIXME
  if (pbuf != NULL) {
    if (ptr->pscInput(pbuf, argv)) pbuf_free(pbuf);
  }
} while(pbuf!=NULL);

return 0;
}

/**	----------------------------------------------------------------------------
	* @brief  Функция-обработчик выходных пакетов
	* @param  netif: Указатель упр. структуру
	* @param  p: Указатель на заголовок сообщения
	* @param  argv: Указатель на всё
	* @retval pbuf*: Указатель на заголовок сообщения */
err_t 
  net_eth__output(void *parg1, struct pbuf *p, void * parg2) {
/*----------------------------------------------------------------------------*/
  err_t errval;
  struct pbuf *q;
  uint32_t framelength = 0;
  uint32_t bufferoffset = 0;
  uint32_t byteslefttocopy = 0;
  uint32_t payloadoffset = 0;
  __IO ETH_DMADescTypeDef *pdmatx;
  ETH_HandleTypeDef *phandle;
  uint8_t *buffer;
	//net_eth_t *pxEth;
  
  // проверка
  if ((!parg2)||(!p)) return ERR_VAL;
  
	net_eth_t *pxeth = (net_eth_t *)parg2;
  //argv достаточно быть ETH_HandleTypeDef 
  phandle = (ETH_HandleTypeDef*)&(pxeth->xHandle);
  pdmatx = phandle->TxDesc;
  buffer = (uint8_t *)(pdmatx->Buffer1Addr);
  bufferoffset = 0;
  
  /* copy frame from pbufs to driver buffers */
  for(q = p; q != NULL; q = q->next)
  {
    /* Is this buffer available? If not, goto error */
    if((pdmatx->Status & ETH_DMATXDESC_OWN) != (uint32_t)RESET)
    {
      errval = ERR_USE;
      goto error;
    }
    
    /* Get bytes in current lwIP buffer */
    byteslefttocopy = q->len;
    payloadoffset = 0;
    
    /* Check if the length of data to copy is bigger than Tx buffer size*/
    while( (byteslefttocopy + bufferoffset) > ETH_TX_BUF_SIZE )
    {
      /* Copy data to Tx buffer*/
      memcpy( (uint8_t*)((uint8_t*)buffer + bufferoffset), 
				(uint8_t*)((uint8_t*)q->payload + payloadoffset), 
				(ETH_TX_BUF_SIZE - bufferoffset) );
      
      /* Point to next descriptor */
      pdmatx = (ETH_DMADescTypeDef *)(pdmatx->Buffer2NextDescAddr);
      
      /* Check if the buffer is available */
      if((pdmatx->Status & ETH_DMATXDESC_OWN) != (uint32_t)RESET)
      {
        errval = ERR_USE;
        goto error;
      }
      
      buffer = (uint8_t *)(pdmatx->Buffer1Addr);
      
      byteslefttocopy = byteslefttocopy - (ETH_TX_BUF_SIZE - bufferoffset);
      payloadoffset = payloadoffset + (ETH_TX_BUF_SIZE - bufferoffset);
      framelength = framelength + (ETH_TX_BUF_SIZE - bufferoffset);
      bufferoffset = 0;
    }
    
    /* Copy the remaining bytes */
    memcpy( (uint8_t*)((uint8_t*)buffer + bufferoffset), 
			(uint8_t*)((uint8_t*)q->payload + payloadoffset), byteslefttocopy );
    bufferoffset = bufferoffset + byteslefttocopy;
    framelength = framelength + byteslefttocopy;
  }
  
  /* Prepare transmit descriptors to give to DMA */ 
  HAL_ETH_TransmitFrame(phandle, framelength);
  
  errval = ERR_OK;
  
error:
  
  /* When Transmit Underflow flag is set, clear it and issue a Transmit Poll Demand to resume transmission */
  if ((phandle->Instance->DMASR & ETH_DMASR_TUS) != (uint32_t)RESET)
  {
    /* Clear TUS ETHERNET DMA flag */
    phandle->Instance->DMASR = ETH_DMASR_TUS;
    
    /* Resume DMA transmission*/
    phandle->Instance->DMATPDR = 0;
  }
  return errval;
}

// Локальные (static) функции

/**	----------------------------------------------------------------------------
	* @brief  Функция-обработчик входных пакетов
	* @param  argv: Указатель на всё
	* @retval pbuf*: Указатель на заголовок сообщения */
static struct pbuf * 
	read_bytes(ETH_HandleTypeDef *phandle) {
/*----------------------------------------------------------------------------*/	
  struct pbuf *p = NULL, *q = NULL;
  uint16_t len = 0;
  uint8_t *buffer;
  __IO ETH_DMADescTypeDef *dmarxdesc;
  uint32_t bufferoffset = 0;
  uint32_t payloadoffset = 0;
  uint32_t byteslefttocopy = 0;
  uint32_t i=0;
  //ETH_HandleTypeDef *phandle;
  
  // проверка
  if (!phandle) return NULL;
	
	//phandle = &(((net_eth_t *)argv)->xHandle);
  
  /* get received frame */
  if(HAL_ETH_GetReceivedFrame_IT(phandle) != HAL_OK)
    return NULL;
  
  /* Obtain the size of the packet and put it into the "len" variable. */
  len = phandle->RxFrameInfos.length;
  buffer = (uint8_t *)phandle->RxFrameInfos.buffer;
  
  if (len > 0)
  {
    /* We allocate a pbuf chain of pbufs from the Lwip buffer pool */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  }
  
  if (p != NULL)
  {
    dmarxdesc = phandle->RxFrameInfos.FSRxDesc;
    bufferoffset = 0;
    
    for(q = p; q != NULL; q = q->next)
    {
      byteslefttocopy = q->len;
      payloadoffset = 0;
      
      /* Check if the length of bytes to copy in current pbuf is bigger than Rx buffer size */
      while( (byteslefttocopy + bufferoffset) > ETH_RX_BUF_SIZE )
      {
        /* Copy data to pbuf */
        memcpy( (uint8_t*)((uint8_t*)q->payload + payloadoffset), (uint8_t*)((uint8_t*)buffer + bufferoffset), (ETH_RX_BUF_SIZE - bufferoffset));
        
        /* Point to next descriptor */
        dmarxdesc = (ETH_DMADescTypeDef *)(dmarxdesc->Buffer2NextDescAddr);
        buffer = (uint8_t *)(dmarxdesc->Buffer1Addr);
        
        byteslefttocopy = byteslefttocopy - (ETH_RX_BUF_SIZE - bufferoffset);
        payloadoffset = payloadoffset + (ETH_RX_BUF_SIZE - bufferoffset);
        bufferoffset = 0;
      }
      
      /* Copy remaining data in pbuf */
      memcpy( (uint8_t*)((uint8_t*)q->payload + payloadoffset), (uint8_t*)((uint8_t*)buffer + bufferoffset), byteslefttocopy);
      bufferoffset = bufferoffset + byteslefttocopy;
    }
  }
    
  /* Release descriptors to DMA */
  /* Point to first descriptor */
  dmarxdesc = phandle->RxFrameInfos.FSRxDesc;
  /* Set Own bit in Rx descriptors: gives the buffers back to DMA */
  for (i=0; i< phandle->RxFrameInfos.SegCount; i++)
  {  
    dmarxdesc->Status |= ETH_DMARXDESC_OWN;
    dmarxdesc = (ETH_DMADescTypeDef *)(dmarxdesc->Buffer2NextDescAddr);
  }
    
  /* Clear Segment_Count */
  phandle->RxFrameInfos.SegCount =0;
  
  /* When Rx Buffer unavailable flag is set: clear it and resume reception */
  if ((phandle->Instance->DMASR & ETH_DMASR_RBUS) != (uint32_t)RESET)  
  {
    /* Clear RBUS ETHERNET DMA flag */
    phandle->Instance->DMASR = ETH_DMASR_RBUS;
    /* Resume DMA reception */
    phandle->Instance->DMARPDR = 0;
  }
  return p;
}
