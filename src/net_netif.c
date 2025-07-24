#include "net_netif.h"
#include "cmsis_os.h"
#include "lwip/dhcp.h"
#include "netif/etharp.h"
#include "string.h"
#include "proj_conf.h"

static void check_link( void * );
static void	task_input( void *pargv );
static void link_callback( net_netif_t * );
static err_t ethif_netif_setup( struct netif * );
static err_t input_wrapper(struct pbuf *, void *);
static inline err_t	output( struct netif *, struct pbuf *, void * );
static void link_callback_wrapper( struct netif *, void * );
static bool is_link_up(void);
static bool is_netif_up(void);

static net_netif_t xNetNetif; // инстанс среднего 'Netif' уровня

extern const uint8_t macaddress[6];

int heap_size;

// Общедоступные (public) функции

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
	* @brief  Функция-обработчик входных пакетов в ответ на сигнал семафора
	* @param  argv: Указатель на всё
	* @retval none: Нет */
s32_t
	net_netif__init(net_netif_t *px, netif_is_up_ptr_t callback) {
/*----------------------------------------------------------------------------*/
	s32_t rc;

	((net_eth_t *)net_eth__inst())->pscInput = input_wrapper;
	// Инициализ. Eth вызывается для всех случаев, будь-то линк вкл. или откл.
	// Все основные регистры HAL_ETH_Init успевает проинициализировать до возврата
	// статуса HAL_TIMEOUT
	rc = net_eth__init(net_eth__inst());
  
	// Ставим функцию для обратной связи при изменении линка
	netif_set_link_callback(&(px->xNetif), link_callback_wrapper);
  //
	px->pvNetifCb = callback;
  
	// выбираем тип адресации (TODO Не реализован DHCP)
	//if (false)	//TODO
	//	pxEth->Ip.eType = IPDYNAMIC;
	//else
	px->xIp.eType = IPSTATIC;
	// если выбран dchp, то выставляем его в off.
	px->xIp.xDynamic.eState = DHCP_OFF;
	// для статики запиываем принудительный адрес
  return rc; // нельзя возвращать сразу после net_eth__init! FIXME бесполезная
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

// Локальные (static) функции

/**	----------------------------------------------------------------------------
	* @brief  Функция-обработчик входных пакетов в ответ на сигнал семафора
	* @param  argv: Указатель на всё */
static void
	check_link( void *pargv ) {
/*----------------------------------------------------------------------------*/
	s32_t result = 0;
	net_netif_t *px;
  s32_t rc;
	static bool bLink;

	px = (net_netif_t *)pargv;

	do {
		// Чтение регистра BSR через аппаратный интерфейс RMII. Если чтение прошло 
		// успешно, то смотрим на PHY Link
    rc = net_eth__is_links_up(net_eth__inst(), &bLink);
    //rc = net_eth__is_link_up(net_eth__inst(), &bLink);
    if (rc < 0) {
      // Проблема со связью с микросхемой PHY. Что делать?
    }
		if ( bLink ) {
			// Если PHY Link поднят, то смотрим смотрим на NETIF Link
			if (!(is_link_up())) {
				// Если NETIF Link сброшен, то поднимаем его
				netif_set_link_up(&(px->xNetif), (void*)px);
				// Проверяем тип адресации
				if (px->xIp.eType == IPDYNAMIC) {
#if (LWIP_DHCP==1)
          // Если тип адресации динамическая, то запускаем dhcp-клиент, если 
					// он еще не запущен
					if (px->xIp.xDynamic.eState == DHCP_OFF) {
						if (dhcp_start(&(px->xNetif)) == ERR_OK)
							px->xIp.xDynamic.eState = DHCP_WAIT_ADDRESS;
						else
							px->xIp.xDynamic.eState = DHCP_ERROR;
					}
#endif // LWIP_DHCP
				} else if (px->xIp.eType == IPSTATIC) {
					// Если тип адресации статическая, то пока ничего
					__NOP();
				}
			} else {
				// Если NETIF Link поднят, то снова проверяем адресацию и запускаем сервер
				if (px->xIp.eType == IPDYNAMIC) {
#if (LWIP_DHCP==1)
					// Смотрим на состояние dhcp-клиента
					if (px->xIp.xDynamic.eState == DHCP_WAIT_ADDRESS) {
						// Если dhcp-клиент только ждёт получения адреса, то вызываем каждый
						// раз функцию dhcp_supplied_address.
						if (dhcp_supplied_address(&(px->xNetif))) {
							px->xIp.xDynamic.eState = DHCP_ADDRESS_ASSIGNED;
							// запуск Modbus-TCP
							if (px->pvNetifCb) px->pvNetifCb(is_netif_up());
						} else {
							struct dhcp * pdhcp = (struct dhcp *)netif_get_client_data(&(px->xNetif), LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);
							if (pdhcp->tries > MAX_DHCP_TRIES) {
								px->xIp.xDynamic.eState = DHCP_TIMEOUT;
								dhcp_stop(&(px->xNetif));
							}
						}
					} else if (px->xIp.xDynamic.eState == DHCP_ADDRESS_ASSIGNED) {
						// ничего не делаем
						__NOP();
					}
#endif // LWIP_DHCP
				} else if (px->xIp.eType == IPSTATIC) {
					// Вызвать сервер на исполнение, если он еще не запущен
					if (px->pvNetifCb) px->pvNetifCb(is_netif_up());
				}
			}
		}
		// Если PHY Link сброшен, то ...
		else {
			if (is_link_up()) {
				// если линк активен, то выключаем его
				if (px->xIp.eType == IPDYNAMIC) {
#if (LWIP_DHCP==1)
					px->xIp.xDynamic.eState = DHCP_OFF;
					dhcp_stop(&(px->xNetif));
#endif // LWIP_DHCP
				}
				// Сбрасываем NETIF Link
				netif_set_link_down(&(px->xNetif), (void*)px);
				// Выгрузка Modbus-TCP
				if (px->pvNetifCb) px->pvNetifCb(is_netif_up());
			} else {
				// если линк уже сброшен, то ничего не делаем
				__NOP();
			}
		}
		// 2. Задержка на период опроса 1000 мсек.
    heap_size = xPortGetFreeHeapSize();
    vTaskDelay(1000);
    //printf("check_link is working");
	} while (result == 0);
  
	// Удаляем задачу
	vTaskDelete(NULL);
}

/**	----------------------------------------------------------------------------
	* @brief  Функция-обработчик входных пакетов в ответ на сигнал семафора
	* @param  argv: Указатель на всё
	* @retval none: Нет */
static void
	task_input( void *pargv ) {
/*----------------------------------------------------------------------------*/
	s32_t result;
	net_netif_t *px = (net_netif_t *)pargv;

	// шаг 2/3. Запускаем цикл чтения очередного входящего пакета до тех пор...
	do {
		// Ожидаем приема нового байта. Блокируемся.
		xSemaphoreTake(px->pvSmphrInput, portMAX_DELAY);	
		// Читаем данные и сообщаем о результате...
		result = net_eth__input(net_eth__inst(), &(px->xNetif));
		__NOP();
	// ... до тех пор, пока результат выполнения 0
	} while (result == 0);

	// Удаляем задачу
	vTaskDelete(NULL);
}

/**	----------------------------------------------------------------------------
	* @brief Функция обратного вызова, которая вызывается при изменении статуса 
	*		линка RJ-45 (возможны два варианта: активный или неактивный)
	* @param netif: Указатель на упр.структуру
	* @param argv: Указатель на всё
	* @retval none: Нет */
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
		
		// 2. создать семафоры на очередной вход.байт и на извещение о закрытии стека
		px->pvSmphrInput = xSemaphoreCreateBinary();
		
		// 3. запустить поток input
		xTaskCreate( task_input, 
                 "EthIf",
                 (2*configMINIMAL_STACK_SIZE),
                 px,
                 makeFreeRtosPriority(osPriorityRealtime),
                 &(px->pvTaskInput) );
		
		// 4. запустить периферию
		net_eth__start(net_eth__inst());

		// Индикация что линк активен
		LWIP_DEBUGF( NET_DEBUG, ("ETH has started, in '%s' /NET/net_netif.c:%d\r\n", 
      __FUNCTION__, __LINE__) );
	} else {
		
		// 4.
		net_eth__stop(net_eth__inst());
		
		// 1.
		netif_set_down(&(px->xNetif));
		netif_remove(&(px->xNetif));
		
		// 3.
		vTaskDelete(px->pvTaskInput);
		
		// 2.
		vSemaphoreDelete(px->pvSmphrInput);
		
		// Индикация что линк нективен
    LWIP_DEBUGF( NET_DEBUG, ("ETH has stopped, in '%s' /NET/net_netif.c:%d\r\n",
      __FUNCTION__, __LINE__) );
	}
}

/**	----------------------------------------------------------------------------
	* @brief  Калибровка
	* @param  netif: Указатель на управляющую структуру
	* @retval err_t: Результат выполнения функции */
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
	* @brief  Функция-обработчик входных пакетов в ответ на сигнал семафора
	* @param  argv: Указатель на всё
	* @retval none: Нет */
static inline err_t
	output( struct netif *netif, struct pbuf *p, void * argv ) {
/*----------------------------------------------------------------------------*/
  return net_eth__output(NULL, p, argv);
}

/**	----------------------------------------------------------------------------
	* @brief  Функция-обработчик входных пакетов в ответ на сигнал семафора
	* @param  argv: Указатель на всё
	* @retval none: Нет */
static err_t
	input_wrapper(struct pbuf *p, void *pargv) {
/*----------------------------------------------------------------------------*/
	struct netif *pnetif = (struct netif *)pargv;
	return pnetif->input(p, pnetif);
}

/**	----------------------------------------------------------------------------
	* @brief  Функция-обработчик входных пакетов в ответ на сигнал семафора
	* @param  argv: Указатель на всё
	* @retval none: Нет */
static void
	link_callback_wrapper( struct netif *netif, void * argv ) {
/*----------------------------------------------------------------------------*/
  link_callback((net_netif_t *)argv);
}

/**	----------------------------------------------------------------------------
	* @brief  ???
	* @param  argv: Указатель на всё
	* @retval none: Нет */
void
	net_netif__sig_input( net_netif_t *px ) {
/*----------------------------------------------------------------------------*/
  long lSwitchingIsNeeded;

	xSemaphoreGiveFromISR( px->pvSmphrInput, &lSwitchingIsNeeded );
	portEND_SWITCHING_ISR(lSwitchingIsNeeded);
}

