/**
  ******************************************************************************
  * @file           : hwConfig.h
  * @brief          : Code to configure the MCU devices (hardware)
  ******************************************************************************
  */
#ifndef __HWCONFIG_H__
#define __HWCONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif
	
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal_conf.h"
#include <usbd_core.h>
#include <usbd_cdc.h>
#include "usbd_cdc_if.h"
#include <usbd_desc.h>

/* Exported variables -------------------------------------------------------*/
extern ADC_HandleTypeDef hadc1;
extern SPI_HandleTypeDef hspi2;

extern DAC_HandleTypeDef hdac;
extern ADC_HandleTypeDef hadc2;
extern LTDC_HandleTypeDef hltdc;
extern TIM_HandleTypeDef htim3;
/* Exported functions prototypes ---------------------------------------------*/
void SystemClock_Config(void);
void InitUsb(void);
void MX_GPIO_Init(void);
void MX_ADC1_Init(void);
void MX_ADC2_Init(void);
void MX_NVIC_Init(void);
void MX_SPI2_Init(void);
	
void MX_DAC_Init(void);
void MX_LTDC_Init(void);
void MX_TIM3_Init(void);

/* Defines ---------------------------------------------------------------------*/	
#define Pin_LedGreen						GPIO_PIN_13
#define Pin_Button							GPIO_PIN_0
#define GPIO_Port_Button					GPIOA
	
#define Pin_DacOut2							GPIO_PIN_5
#define	GPIO_Port_DacOut2					GPIOA
#define Pin_Adc1In1							GPIO_PIN_1
#define GPIO_Port_Adc1In1					GPIOA
#define Pin_Adc1In13						GPIO_PIN_3
#define GPIO_Port_Adc1In13					GPIOC
	
#define Pin_LcdDE							GPIO_PIN_10
#define GPIO_Port_LcdDE						GPIOF
#define Pin_LcdB5							GPIO_PIN_3
#define GPIO_Port_LcdB5						GPIOA
#define Pin_LcdVsync						GPIO_PIN_4
#define GPIO_Port_LcdVsync					GPIOA
#define Pin_LcdG5							GPIO_PIN_6
#define GPIO_Port_LcdG5						GPIOA
#define Pin_LcdR3							GPIO_PIN_0
#define GPIO_Port_LcdR3						GPIOB
#define Pin_LcdR6							GPIO_PIN_1
#define GPIO_Port_LcdR6						GPIOB
#define Pin_LcdG3							GPIO_PIN_11
#define GPIO_Port_LcdG3						GPIOE
#define Pin_LcdB4							GPIO_PIN_12
#define GPIO_Port_LcdB4						GPIOE
#define Pin_LcdClk							GPIO_PIN_14
#define GPIO_Port_LcdClk					GPIOE
#define Pin_LcdR7							GPIO_PIN_15
#define GPIO_Port_LcdR7						GPIOE
#define Pin_LcdG4							GPIO_PIN_10
#define GPIO_Port_LcdG4						GPIOB
#define Pin_LcdG5B11						GPIO_PIN_11
#define GPIO_Port_LcdG5B11					GPIOB
#define Pin_LcdB3							GPIO_PIN_10
#define GPIO_Port_LcdB3						GPIOD
#define Pin_LcdHSync						GPIO_PIN_6
#define GPIO_Port_LcdHSync					GPIOC
#define Pin_LcdG6							GPIO_PIN_7
#define GPIO_Port_LcdG6						GPIOC
#define Pin_LcdR4							GPIO_PIN_11
#define GPIO_Port_LcdR4						GPIOA
#define Pin_LcdR5							GPIO_PIN_12
#define GPIO_Port_LcdR5						GPIOA
#define Pin_LcdG7							GPIO_PIN_3
#define GPIO_Port_LcdG7						GPIOD
#define Pin_LcdB6							GPIO_PIN_8
#define GPIO_Port_LcdB6						GPIOB
#define Pin_LcdB7							GPIO_PIN_9
#define GPIO_Port_LcdB7						GPIOB	
#ifdef __cplusplus
}
#endif

#endif // __HWCONFIG_H__