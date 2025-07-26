#ifndef _PROJ_CONF_CLT_H_
#define _PROJ_CONF_CLT_H_

#include "userint.h"
#include "lwip/opt.h"

// ������� "��� ������" ��� �������������
#define CLT

//#define   _TEST_    // ��� �������� �� �����
#define _TARGET_  // ��� ������ � ����

//#define CLT_IS_407IC
#define CLT_IS_PI396
//#define CLT_IS_PI396IZM1

#define CLT_NUM_MAX                     (4U)
#define SRV_NUM_MAX                     (4U)
#define RMT_CLT_MAX                     (4U)

// ������� ���������
// ���8 (multicast l����� ���� ��������� ��� Wireshark)
#define MAC_ADDR                        { 1U, 0U, 0U, 0U, 0U, 0x10U }
#define IP_ADDR                         "192.168.1.10" // IP-����� ����������
#define MASK_ADDR                       "255.255.255.0"
#define GW_ADDR                         "192.168.1.1"

// Authentication
#define UPVS_MQTT_CLT_CLIENTID          "PVS-55"
#define UPVS_MQTT_CLT_USERNAME          "public"
#define UPVS_MQTT_CLT_PASSWORD          "public"

// ��������� ��� PHY_MAX_LEN=2
// 1 - ��� ����� 407IC, 0 - ��� ����� 396 ���.0, 1,2 - ��� ����� 396 ���.1 
#if   (defined CLT_IS_407IC)
  #define  PHY_ADDR                     { 1U, 0U }
#elif (defined CLT_IS_PI396)
  #define  PHY_ADDR                     { 0U, 0U }
#elif (defined CLT_IS_PI396IZM1)
  #define  PHY_ADDR                     { 1U, 2U }
#endif

// ��������� UART
#if   (defined CLT_IS_407IC)
  #define UART1_DE_PORT                 GPIOE
  #define UART1_DE_PIN                  GPIO_ODR_ODR_2
#elif (defined CLT_IS_PI396)|(defined CLT_IS_PI396IZM1)
  #define UART1_DE_PORT                 GPIOA
  #define UART1_DE_PIN                  GPIO_ODR_ODR_8
#endif

#endif //_PROJ_CONF_CLT_H_