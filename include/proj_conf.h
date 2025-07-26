#ifndef _PROJ_CONF_CLT_H_
#define _PROJ_CONF_CLT_H_

#include "userint.h"
#include "lwip/opt.h"

// Команда "это клиент" для препроцессора
#define CLT

//#define   _TEST_    // для проверка на столе
#define _TARGET_  // для работы в цехе

//#define CLT_IS_407IC
#define CLT_IS_PI396
//#define CLT_IS_PI396IZM1

#define CLT_NUM_MAX                     (4U)
#define SRV_NUM_MAX                     (4U)
#define RMT_CLT_MAX                     (4U)

// Сетевые настройки
// бит8 (multicast lолжен быть выставлен для Wireshark)
#define MAC_ADDR                        { 1U, 0U, 0U, 0U, 0U, 0x10U }
#define IP_ADDR                         "192.168.1.10" // IP-адрес устройства
#define MASK_ADDR                       "255.255.255.0"
#define GW_ADDR                         "192.168.1.1"

// Authentication
#define UPVS_MQTT_CLT_CLIENTID          "PVS-55"
#define UPVS_MQTT_CLT_USERNAME          "public"
#define UPVS_MQTT_CLT_PASSWORD          "public"

// адресация для PHY_MAX_LEN=2
// 1 - для платы 407IC, 0 - для платы 396 изм.0, 1,2 - для платы 396 изм.1 
#if   (defined CLT_IS_407IC)
  #define  PHY_ADDR                     { 1U, 0U }
#elif (defined CLT_IS_PI396)
  #define  PHY_ADDR                     { 0U, 0U }
#elif (defined CLT_IS_PI396IZM1)
  #define  PHY_ADDR                     { 1U, 2U }
#endif

// Настрйоки UART
#if   (defined CLT_IS_407IC)
  #define UART1_DE_PORT                 GPIOE
  #define UART1_DE_PIN                  GPIO_ODR_ODR_2
#elif (defined CLT_IS_PI396)|(defined CLT_IS_PI396IZM1)
  #define UART1_DE_PORT                 GPIOA
  #define UART1_DE_PIN                  GPIO_ODR_ODR_8
#endif

#endif //_PROJ_CONF_CLT_H_