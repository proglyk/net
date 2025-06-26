#ifndef _CONFIG_PROJ_SRV_H_
#define _CONFIG_PROJ_SRV_H_

#include "userint.h"
#include "config_lib.h"

// ������� "��� ������" ��� �������������
//#define _SRV_

//#define   _TEST_    // ��� �������� �� �����
#define _TARGET_  // ��� ������ � ����

//#define SERV_IS_407IC
#define   SERV_IS_PI396

#define CLT_NUM_MAX                     (4U)
#define SRV_NUM_MAX                     (4U)
#define RMT_CLT_MAX                     (4U)

// ����� ���������� ���-���
// 1) �� UART3 ��� ����� 407IC, 2) � ���� "Terminal" ��� PI396/PI396IZM1
// �� ������ ������ ������������ define ������ � �������, ����� �� ��������
//#define NET_DEBUG                       LWIP_DBG_OFF
//#define DBG_ON                          LWIP_DBG_ON // ������ �������� NET_DEBUG

//#define DBG_TERMINAL                    1
//#define DBG_SHELL                       1

//#error "fuck "you"

// ������� ���������
// ���8 (multicast l����� ���� ��������� ��� Wireshark)
#define MAC_ADDR                        { 1U, 0U, 0U, 0U, 0U, 0x11U }
#define IP_ADDR                         "192.168.1.12"
#define MASK_ADDR                       "255.255.255.0"
#define GW_ADDR                         "192.168.1.1"


// ��������� ��� PHY_MAX_LEN=2
// 1 - ��� ����� 407IC, 0 - ��� ����� 396
#if   (defined SERV_IS_407IC)
  #define  PHY_ADDR                     { 1U, 0U }
#elif (defined SERV_IS_PI396)
  #define  PHY_ADDR                     { 0U, 0U }
#endif

// 1 - ��� ����� 407IC, 0 - ��� ����� 396
// #if defined SERV_IS_407IC
  // #define  PHY_ADDR                     (1L) 
// #elif defined SERV_IS_PI396
  // #define  PHY_ADDR                     (0L)
// #endif

// ��������� UART
#if defined SERV_IS_407IC
  #define UART1_DE_PORT                 GPIOE
  #define UART1_DE_PIN                  GPIO_ODR_ODR_2
#elif defined SERV_IS_PI396
  #define UART1_DE_PORT                 GPIOA
  #define UART1_DE_PIN                  GPIO_ODR_ODR_8
#endif

#endif //_CONFIG_PROJ_SRV_H_