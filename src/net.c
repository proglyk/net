#include "net.h"
#include "stdlib.h"
#include "string.h"
#include "net_netif.h"
#include "lwip/tcpip.h"
#include "lwip/ip_addr.h"
#include "config_proj.h"

// ��������� ��������� (static) �������

static int get_free_pos_srv(net_srv_t *pxSrv);
static int get_free_pos_clt(net_clt_t *pxClt);
static void	netif_callback(bool);

// ���������� ��������� (static)

static net_t xNet; // ������� �������� 'Net' ������


// ��������� (public) �������

/**	----------------------------------------------------------------------------
	* @brief ??? */
s32_t
	net__init(net_t * pnet) {
/*----------------------------------------------------------------------------*/
	int rc=0;
  rc = net_netif__init(net_netif__inst(), netif_callback);
  if (rc < 0) return rc;
	tcpip_init(NULL, NULL);
  return 0;
}

/**	----------------------------------------------------------------------------
	* @brief ???
	* @param pnet: ??? */
void
	net__run(net_t * pnet) {
/*----------------------------------------------------------------------------*/
	//
  xNet.xClt.bNetifIsUp = xNet.xSrv.bNetifIsUp = false;
  // ��������� ������ ��������, ��������
  net_clt__task_create(&(xNet.xClt));
  net_srv__task_create(&(xNet.xSrv));
  //
  net_netif__task_create(net_netif__inst());
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
s32_t
	net__add_srv(net_t * pNet, net_init_t *pInit) {
/*----------------------------------------------------------------------------*/
  s32_t pos = 0; //FIXME
  
  // ����� ��������� ������� � ������� axServers
  if ((pos = get_free_pos_srv(&(pNet->xSrv))) < 0) {
    return -1;
  }
  // �������� ������ �� ���������������� ���������
  strcpy((char *)pNet->xSrv.axServers[pos].acName, pInit->pcName);
  pNet->xSrv.axServers[pos].pxFn = pInit->pxFn;
  pNet->xSrv.axServers[pos].bEnable = pInit->bEnabled;
  pNet->xSrv.axServers[pos].ulPort = pInit->ulPort;
  // ����������� ��������� ��������� ��������
  pNet->xSrv.axServers[pos].slTaskCounter = 0;
  pNet->xSrv.axServers[pos].slSockServ = -1;
  for (u8_t j=0; j<RMT_CLT_MAX; j++) {
    pNet->xSrv.axServers[pos].axRmtClt[j].xData.slSock = -1;
  }
  
	return 0;
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
s32_t
	net__add_clt(net_t * pNet, net_init_t *pInit) {
/*----------------------------------------------------------------------------*/
  s32_t pos = 0; //FIXME
	u32_t rc;
  
  // ����� ��������� ������� � ������� axClients
  if ((pos = get_free_pos_clt(&(pNet->xClt))) < 0) {
    return -1;
  }
  // �������� ������ �� ���������������� ���������
  strcpy((char *)pNet->xClt.axClients[pos].acName, pInit->pcName);
  pNet->xClt.axClients[pos].pxFn = pInit->pxFn;
  pNet->xClt.axClients[pos].bEnable = pInit->bEnabled;
  pNet->xClt.axClients[pos].ulSockType = (pInit->eSockType == CLT_TCP) ? 
                                         (SOCK_STREAM) : (SOCK_DGRAM);
  rc = ipaddr_aton( (const char *)pInit->pcRmt, 
										&(pNet->xClt.axClients[pos].xIpRmt) );
  if (!rc) return -1;
  pNet->xClt.axClients[pos].ulPort = pInit->ulPort;
  // ��������. ������ ��� sntp
  pNet->xClt.axClients[pos].ulId = pInit->ulId;
  
	return 0;
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
net_t *
	net__inst(void) {
/*----------------------------------------------------------------------------*/
	return &xNet;
}

/**	----------------------------------------------------------------------------
	* @brief  ???
	* @param  pctl: ��������� �� �� */
void
	net__irq( net_t *pnet ) {
/*----------------------------------------------------------------------------*/
  net_eth__irq(net_eth__inst());
}

/**	----------------------------------------------------------------------------
	* @brief  ???
	* @param  pctl: ��������� �� �� */
void
	net__input( net_t *pnet ) {
/*----------------------------------------------------------------------------*/
  net_netif__sig_input(net_netif__inst());
}

// ��������� (static) �������

/**	----------------------------------------------------------------------------
	* @brief ??? */
static int
	get_free_pos_srv(net_srv_t *pxSrv) {
/*----------------------------------------------------------------------------*/
  for (u8_t i = 0; i < SRV_NUM_MAX; i++) {
		if (pxSrv->axServers+i) {
			if ( !strcmp((const char *)pxSrv->axServers[i].acName, "") ) {
				return i;
			}
		}
	}
  return -1;
}

/**	----------------------------------------------------------------------------
	* @brief ??? */
static int
	get_free_pos_clt(net_clt_t *pxClt) {
/*----------------------------------------------------------------------------*/
  for (u8_t i = 0; i < CLT_NUM_MAX; i++) {
		if (pxClt->axClients+i) {
			if ( !strcmp((const char *)pxClt->axClients[i].acName, "") ) {
				return i;
			}
		}
	}
  return -1;
}

/**	----------------------------------------------------------------------------
	* @brief  ???
	* @param  bool: ��������� �� �� */
static void
	netif_callback( bool sta_new ) {
/*----------------------------------------------------------------------------*/
  static bool sta_old = false;
  
  // ���������� ����� ��������� �� ������
  if (sta_new != sta_old) {
    // ���������� ������ ����� � ����������� SRV � CLT
    xNet.xClt.bNetifIsUp = xNet.xSrv.bNetifIsUp = sta_new;
    // ������ ��� "link is down" ��������� �������.
    if (sta_new == false)
      net_srv__delete_all(&(xNet.xSrv));
    // ������� � �������
    LWIP_DEBUGF( NET_DEBUG, ("Link '%s', in '%s' /NET/net.c:%d\r\n", 
      sta_new == true ? "is up" : "is down", __FUNCTION__, __LINE__) );
    // ��������� ����� ���������
    sta_old = sta_new;
  }
}
