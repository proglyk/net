#include "net_netif.h"
#include "cmsis_os.h"
#include "lwip/dhcp.h"
#include "netif/etharp.h"
#include "string.h"
#include "proj_conf.h"
//#include "net_user.h"

static void check_link( void * );
static void	task_input( void *pargv );
static void link_callback( net_netif_t * );
static err_t ethif_netif_setup( struct netif * );
static err_t input_wrapper(struct pbuf *, void *);
static inline err_t	output( struct netif *, struct pbuf *, void * );
static void link_callback_wrapper( struct netif *, void * );
static bool is_link_up(void);
static bool is_netif_up(void);

static net_netif_t xNetNetif; // ������� �������� 'Netif' ������

extern const uint8_t macaddress[6];

int heap_size;

// ������������� (public) �������

/**	----------------------------------------------------------------------------
	* @brief ??? */
net_netif_t *
	net_netif__inst(void) {
/*----------------------------------------------------------------------------*/
	return &xNetNetif;
}

/**	----------------------------------------------------------------------------
	* @brief ???
	* @param pnet: ??? */
void
	net_netif__task_create( net_netif_t *px ) {
/*----------------------------------------------------------------------------*/
	xTaskCreate( check_link, 
               "eth_link", 
               4*configMINIMAL_STACK_SIZE, 
               px, 
               makeFreeRtosPriority(osPriorityNormal), 
               &(px->pvTaskLink) );
}

/**	----------------------------------------------------------------------------
	* @brief  �������-���������� ������� ������� � ����� �� ������ ��������
	* @param  argv: ��������� �� ��
	* @retval none: ��� */
s32_t
	net_netif__init(net_netif_t *px, netif_is_up_ptr_t callback) {
/*----------------------------------------------------------------------------*/
	s32_t rc;
	//px->xEthif.pscInput = input_wrapper;
	((net_eth_t *)net_eth__inst())->pscInput = input_wrapper;
	// ���������. Eth ���������� ��� ���� �������, ����-�� ���� ���. ��� ����.
	// ��� �������� �������� HAL_ETH_Init �������� ������������������� �� ��������
	// ������� HAL_TIMEOUT
	rc = net_eth__init(net_eth__inst());
	//if ( rc < 0) return;
	// ������ ������� ��� �������� ����� ��� ��������� �����
	netif_set_link_callback(&(px->xNetif), link_callback_wrapper);
  //
	px->pvNetifCb = callback;
  
	// �������� ��� ���������
	//if (false)	//TODO
	//	pxEth->Ip.eType = IPDYNAMIC;
	//else
	px->xIp.eType = IPSTATIC;
	// ���� ������ dchp, �� ���������� ��� � off.
	px->xIp.xDynamic.eState = DHCP_OFF;
	// ��� ������� ��������� �������������� �����
  return rc; // ������ ���������� ����� ����� net_eth__init! FIXME �����������
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
ip_addr_t *
	net_netif__get_addr(void) {
/*----------------------------------------------------------------------------*/
  return &(net_netif__inst()->xNetif.ip_addr);
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
static bool
	is_netif_up(void) {
/*----------------------------------------------------------------------------*/
  return netif_is_up(&(net_netif__inst()->xNetif));
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
static bool
	is_link_up(void) {
/*----------------------------------------------------------------------------*/
  return netif_is_link_up(&(net_netif__inst()->xNetif));
}

// ��������� (static) �������

/**	----------------------------------------------------------------------------
	* @brief  �������-���������� ������� ������� � ����� �� ������ ��������
	* @param  argv: ��������� �� �� */
static void
	check_link( void *pargv ) {
/*----------------------------------------------------------------------------*/
	s32_t result = 0;
	//s32_t status;
	net_netif_t *px;
	//err_t err;
	struct dhcp * pdhcp;
  s32_t rc;
	static bool bLink;

	px = (net_netif_t *)pargv;

	do {
		// ������ �������� BSR ����� ���������� ��������� RMII. ���� ������ ������ 
		// �������, �� ������� �� PHY Link
    rc = net_eth__is_links_up(net_eth__inst(), &bLink);
    //rc = net_eth__is_link_up(net_eth__inst(), &bLink);
    if (rc < 0) {
      // �������� �� ������ � ����������� PHY. ��� ������?
    }
		if ( bLink ) {
			// ���� PHY Link ������, �� ������� ������� �� NETIF Link
			if (!(is_link_up())) {
				// ���� NETIF Link �������, �� ��������� ���
				netif_set_link_up(&(px->xNetif), (void*)px);
				// ��������� ��� ���������
				if (px->xIp.eType == IPDYNAMIC) {
#if (LWIP_DHCP==1)
          // ���� ��� ��������� ������������, �� ��������� dhcp-������, ���� 
					// �� ��� �� �������
					if (px->xIp.xDynamic.eState == DHCP_OFF) {
						if (dhcp_start(&(px->xNetif)) == ERR_OK)
							px->xIp.xDynamic.eState = DHCP_WAIT_ADDRESS;
						else
							px->xIp.xDynamic.eState = DHCP_ERROR;
					}
#endif // LWIP_DHCP
				} else if (px->xIp.eType == IPSTATIC) {
					// ���� ��� ��������� �����������, �� ���� ������
					__NOP();
				}
			} else {
				// ���� NETIF Link ������, �� ����� ��������� ��������� � ��������� ������
				if (px->xIp.eType == IPDYNAMIC) {
#if (LWIP_DHCP==1)
					// ������� �� ��������� dhcp-�������
					if (px->xIp.xDynamic.eState == DHCP_WAIT_ADDRESS) {
						// ���� dhcp-������ ������ ��� ��������� ������, �� �������� ������
						// ��� ������� dhcp_supplied_address.
						if (dhcp_supplied_address(&(px->xNetif))) {
							px->xIp.xDynamic.eState = DHCP_ADDRESS_ASSIGNED;
							// ������ Modbus-TCP
							if (px->pvNetifCb) px->pvNetifCb(is_netif_up());
						} else {
							pdhcp = (struct dhcp *)netif_get_client_data(&(px->xNetif), LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);
							if (pdhcp->tries > MAX_DHCP_TRIES) {
								px->xIp.xDynamic.eState = DHCP_TIMEOUT;
								dhcp_stop(&(px->xNetif));
							}
						}
					} else if (px->xIp.xDynamic.eState == DHCP_ADDRESS_ASSIGNED) {
						// ������ �� ������
						__NOP();
					}
#endif // LWIP_DHCP
				} else if (px->xIp.eType == IPSTATIC) {
					// ������� ������ �� ����������, ���� �� ��� �� �������
					/*if (pxeth->bRunned == false)*/ {
						// ������ Modbus-TCP
						if (px->pvNetifCb) px->pvNetifCb(is_netif_up());
					}
				}
			}
		}
		// ���� PHY Link �������, �� ...
		else {
			if (is_link_up()) {
				// ���� ���� �������, �� ��������� ���
				if (px->xIp.eType == IPDYNAMIC) {
#if (LWIP_DHCP==1)
					px->xIp.xDynamic.eState = DHCP_OFF;
					dhcp_stop(&(px->xNetif));
#endif // LWIP_DHCP
				}
				// ���������� NETIF Link
				netif_set_link_down(&(px->xNetif), (void*)px);
				// �������� Modbus-TCP
				if (px->pvNetifCb) px->pvNetifCb(is_netif_up());
			} else {
				// ���� ���� ��� �������, �� ������ �� ������
				__NOP();
			}
		}
		// 2. �������� �� ������ ������ 1000 ����.
    heap_size = xPortGetFreeHeapSize();
    vTaskDelay(1000);
    //printf("check_link is working");
	} while (result == 0);
  
	// ������� ������
	vTaskDelete(NULL);
}

/**	----------------------------------------------------------------------------
	* @brief  �������-���������� ������� ������� � ����� �� ������ ��������
	* @param  argv: ��������� �� ��
	* @retval none: ��� */
static void
	task_input( void *pargv ) {
/*----------------------------------------------------------------------------*/
	s32_t result;
	//net_t *pnet;
	net_netif_t *px = (net_netif_t *)pargv;

	//pnet = (net_t *)argv;
	// ��� 2/3. ��������� ���� ������ ���������� ��������� ������ �� ��� ���...
	do {
		// ������� ������ ������ �����. �����������.
		xSemaphoreTake(px->pvSmphrInput, portMAX_DELAY);	
		// ������ ������ � �������� � ����������...
		result = net_eth__input(net_eth__inst()/*&(px->xEthif)*/, &(px->xNetif));
		__NOP();
	// ... �� ��� ���, ���� ��������� ���������� 0
	} while (result == 0);

	// ������� ������
	vTaskDelete(NULL);
}

/**	----------------------------------------------------------------------------
	* @brief ������� ��������� ������, ������� ���������� ��� ��������� ������� 
	*		����� RJ-45 (�������� ��� ��������: �������� ��� ����������)
	* @param netif: ��������� �� ���.���������
	* @param argv: ��������� �� ��
	* @retval none: ��� */
static void
	link_callback(net_netif_t * px) {
/*----------------------------------------------------------------------------*/
	ip_addr_t ipaddr;
	ip_addr_t netmask;
	ip_addr_t gw;
	
	if(netif_is_link_up(&(px->xNetif)))	{
		// 1.0
		switch (px->xIp.eType){
			case IPSTATIC:
        ipaddr_aton(IP_ADDR, &ipaddr);
        ipaddr_aton(MASK_ADDR, &netmask);
        ipaddr_aton(GW_ADDR, &gw);
			break;
			
			case IPDYNAMIC:
				ip_addr_set_zero_ip4(&ipaddr);
				ip_addr_set_zero_ip4(&netmask);
				ip_addr_set_zero_ip4(&gw);
			break;
		}
		
		// 1.1
		netif_add(&(px->xNetif), &ipaddr, &netmask, &gw, (void*)net_eth__inst(), 
			&ethif_netif_setup, &ethernet_input);
		netif_set_default(&(px->xNetif));
		
		// 1.2
		netif_set_up(&(px->xNetif));
		
		// 2. ������� �������� �� ��������� ����.���� � �� ��������� � �������� �����
		px->pvSmphrInput = xSemaphoreCreateBinary();
		
		// 3. ��������� ����� input
		xTaskCreate( task_input, 
                 "EthIf",
                 (2*configMINIMAL_STACK_SIZE),
                 px,
                 makeFreeRtosPriority(osPriorityRealtime),
                 &(px->pvTaskInput) );
		
		// 4. ��������� ���������
		//HAL_ETH_Start(&(px->xHandle));
		net_eth__start(net_eth__inst()/*&(px->xEthif)*/);

		// ��������� ��� ���� �������
		//GPIOG->ODR |= 1<<7;
		LWIP_DEBUGF( NET_DEBUG, ("ETH has started, in '%s' /NET/net_netif.c:%d\r\n", 
      __FUNCTION__, __LINE__) );
	} else {
		
		// 4.
		//HAL_ETH_Stop(&(px->xHandle));
		net_eth__stop(net_eth__inst()/*&(px->xEthif)*/);
		
		// 1.
		netif_set_down(&(px->xNetif));
		netif_remove(&(px->xNetif));
		
		// 3.
		vTaskDelete(px->pvTaskInput);
		
		// 2.
		//if (pxeth->pvSignalInput)
		vSemaphoreDelete(px->pvSmphrInput);
		//vSemaphoreDelete(pxeth->pvSignalStop);
		
		// ��������� ��� ���� ��������
    LWIP_DEBUGF( NET_DEBUG, ("ETH has stopped, in '%s' /NET/net_netif.c:%d\r\n",
      __FUNCTION__, __LINE__) );
	}
}

/**	----------------------------------------------------------------------------
	* @brief  ����������
	* @param  netif: ��������� �� ����������� ���������
	* @retval err_t: ��������� ���������� ������� */
static err_t
	ethif_netif_setup(struct netif *netif) {
/*----------------------------------------------------------------------------*/
	//HAL_StatusTypeDef status;
  //LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
  netif->hostname = "lwip";
#endif

  netif->name[0] = 'X';
  netif->name[1] = 'X';

/* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
  netif->output = etharp_output;
  netif->linkoutput = output;
	
	/* set netif MAC hardware address length */
  netif->hwaddr_len = ETH_HWADDR_LEN;
	
	/* set netif MAC hardware address */
  memcpy(netif->hwaddr, macaddress, sizeof(macaddress));
	
	/* set netif maximum transfer unit */
  netif->mtu = 1500;
	
	/* Accept broadcast address and ARP traffic */
  netif->flags |= /*NETIF_FLAG_BROADCAST |*/ NETIF_FLAG_ETHARP;
	
	return HAL_OK;
}

/**	----------------------------------------------------------------------------
	* @brief  �������-���������� ������� ������� � ����� �� ������ ��������
	* @param  argv: ��������� �� ��
	* @retval none: ��� */
static inline err_t
	output( struct netif *netif, struct pbuf *p, void * argv ) {
/*----------------------------------------------------------------------------*/
  return net_eth__output(NULL, p, argv);
}

/**	----------------------------------------------------------------------------
	* @brief  �������-���������� ������� ������� � ����� �� ������ ��������
	* @param  argv: ��������� �� ��
	* @retval none: ��� */
static err_t
	input_wrapper(struct pbuf *p, void *pargv) {
/*----------------------------------------------------------------------------*/
	struct netif *pnetif = (struct netif *)pargv;
	return pnetif->input(p, pnetif);
}

/**	----------------------------------------------------------------------------
	* @brief  �������-���������� ������� ������� � ����� �� ������ ��������
	* @param  argv: ��������� �� ��
	* @retval none: ��� */
static void
	link_callback_wrapper( struct netif *netif, void * argv ) {
/*----------------------------------------------------------------------------*/
  link_callback((net_netif_t *)argv);
}

/**	----------------------------------------------------------------------------
	* @brief  ???
	* @param  argv: ��������� �� ��
	* @retval none: ��� */
void
	net_netif__sig_input( net_netif_t *px ) {
/*----------------------------------------------------------------------------*/
  long lSwitchingIsNeeded;

	xSemaphoreGiveFromISR( px->pvSmphrInput, &lSwitchingIsNeeded );
	portEND_SWITCHING_ISR(lSwitchingIsNeeded);
}

