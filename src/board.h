/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UART_H
#define __UART_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx.h"

/*****************************************************************
 * @brief Definition for UART ports
 *****************************************************************/ 
#define EVAL_COM1_UART_USB               USART1
#define EVAL_COM1_CLK                    RCC_APB2Periph_USART1

#define EVAL_COM1_TX_PIN                 GPIO_Pin_9
#define EVAL_COM1_TX_GPIO_PORT           GPIOA
#define EVAL_COM1_TX_GPIO_CLK            RCC_AHBPeriph_GPIOA
#define EVAL_COM1_TX_SOURCE              GPIO_PinSource9
#define EVAL_COM1_TX_AF                  GPIO_AF_1

#define EVAL_COM1_RX_PIN                 GPIO_Pin_10
#define EVAL_COM1_RX_GPIO_PORT           GPIOA
#define EVAL_COM1_RX_GPIO_CLK            RCC_AHBPeriph_GPIOA
#define EVAL_COM1_RX_SOURCE              GPIO_PinSource10
#define EVAL_COM1_RX_AF                  GPIO_AF_1
#define EVAL_COM1_IRQn                   USART1_IRQn


#define EVAL_COM2_UART_PCB               USART2
#define EVAL_COM2_CLK                    RCC_APB1Periph_USART2

#define EVAL_COM2_TX_PIN                 GPIO_Pin_14
#define EVAL_COM2_TX_GPIO_PORT           GPIOA
#define EVAL_COM2_TX_GPIO_CLK            RCC_AHBPeriph_GPIOA
#define EVAL_COM2_TX_SOURCE              GPIO_PinSource14
#define EVAL_COM2_TX_AF                  GPIO_AF_1

#define EVAL_COM2_RX_PIN                 GPIO_Pin_15
#define EVAL_COM2_RX_GPIO_PORT           GPIOA
#define EVAL_COM2_RX_GPIO_CLK            RCC_AHBPeriph_GPIOA
#define EVAL_COM2_RX_SOURCE              GPIO_PinSource15
#define EVAL_COM2_RX_AF                  GPIO_AF_1
#define EVAL_COM2_IRQn                   USART2_IRQn

/*****************************************************************
 * @brief Definition for BLDC motor pins and timers
 *****************************************************************/ 
#define BLDC_POWER_PIN									GPIO_Pin_11	
#define BLDC_POWER_GPIO_PORT           	GPIOB
#define BLDC_POWER_GPIO_CLK             RCC_AHBPeriph_GPIOB
#define BLDC_POWER_SOURCE               GPIO_PinSource11
#define BLDC_POWER_AF                   GPIO_AF_0

#define BLDC_DIR_PIN									 	GPIO_Pin_12	
#define BLDC_DIR_GPIO_PORT           	 	GPIOB
#define BLDC_DIR_GPIO_CLK              	RCC_AHBPeriph_GPIOB
#define BLDC_DIR_SOURCE                	GPIO_PinSource12
#define BLDC_DIR_AF                    	GPIO_AF_0

#define BLDC_FG_PIN									 		GPIO_Pin_9	
#define BLDC_FG_GPIO_PORT           	 	GPIOB
#define BLDC_FG_GPIO_CLK              	RCC_AHBPeriph_GPIOB
#define BLDC_FG_SOURCE                	GPIO_PinSource9
#define BLDC_FG_AF                    	GPIO_AF_2

#define BLDC_PWM_PIN									 	GPIO_Pin_5	
#define BLDC_PWM_GPIO_PORT           	 	GPIOB
#define BLDC_PWM_GPIO_CLK              	RCC_AHBPeriph_GPIOB
#define BLDC_PWM_SOURCE                	GPIO_PinSource5
#define BLDC_PWM_AF                    	GPIO_AF_1

#define BLDC_FG_TIMER										TIM17
#define BLDC_FG_TIMER_CLK								RCC_APB2Periph_TIM17
#define BLDC_FG_TIM_CH									TIM_Channel_1
#define BLDC_FG_TIM_IRQ									TIM17_IRQn
#define BLDC_FG_CC											TIM_IT_CC1
#define	BLDC_FG_Update									TIM_IT_Update

#define BLDC_PWM_TIMER									TIM3
#define BLDC_PWM_TIMER_CLK							RCC_APB1Periph_TIM3

/*****************************************************************
 * @brief Definition for STEP motor pins and timers
 *****************************************************************/ 
#define STEP_ENABLE_PIN									GPIO_Pin_10	
#define STEP_ENABLE_GPIO_PORT           GPIOB
#define STEP_ENABLE_GPIO_CLK            RCC_AHBPeriph_GPIOB
#define STEP_ENABLE_SOURCE              GPIO_PinSource10

#define STEP_SLEEP_PIN									GPIO_Pin_6
#define STEP_SLEEP_GPIO_PORT           	GPIOA
#define STEP_SLEEP_GPIO_CLK             RCC_AHBPeriph_GPIOA
#define STEP_SLEEP_SOURCE               GPIO_PinSource6

#define STEP_RESET_PIN									GPIO_Pin_7
#define STEP_RESET_GPIO_PORT           	GPIOA
#define STEP_RESET_GPIO_CLK             RCC_AHBPeriph_GPIOA
#define STEP_RESET_SOURCE               GPIO_PinSource7

#define STEP_DIR_PIN									 	GPIO_Pin_5	
#define STEP_DIR_GPIO_PORT           	 	GPIOA
#define STEP_DIR_GPIO_CLK              	RCC_AHBPeriph_GPIOA
#define STEP_DIR_SOURCE                	GPIO_PinSource5

#define STEP_PWM_PIN									 	GPIO_Pin_4	
#define STEP_PWM_GPIO_PORT           	 	GPIOA
#define STEP_PWM_GPIO_CLK              	RCC_AHBPeriph_GPIOA
#define STEP_PWM_SOURCE                	GPIO_PinSource4
#define STEP_PWM_AF                    	GPIO_AF_4

#define STEP_PWM_TIMER									TIM14
#define STEP_PWM_CLOCK_BASE							SystemCoreClock
#define STEP_PWM_TIMER_CLK							RCC_APB1Periph_TIM14
#define STEP_PWM_TIM_IRQ								TIM14_IRQn
//#define STEP_PWM_CHANNEL								TIM_IT_CC1

#define STEP_MS1_PIN									 	GPIO_Pin_2	
#define STEP_MS1_GPIO_PORT           	 	GPIOB
#define STEP_MS1_GPIO_CLK              	RCC_AHBPeriph_GPIOB
#define STEP_MS1_SOURCE                	GPIO_PinSource2

#define STEP_MS2_PIN									 	GPIO_Pin_1	
#define STEP_MS2_GPIO_PORT           	 	GPIOB
#define STEP_MS2_GPIO_CLK              	RCC_AHBPeriph_GPIOB
#define STEP_MS2_SOURCE                	GPIO_PinSource1

#define STEP_MS3_PIN									 	GPIO_Pin_0	
#define STEP_MS3_GPIO_PORT           	 	GPIOB
#define STEP_MS3_GPIO_CLK              	RCC_AHBPeriph_GPIOB
#define STEP_MS3_SOURCE                	GPIO_PinSource0

/*****************************************************************
 * @brief Definition for SWITCH and GP input pins
 *****************************************************************/ 
#define S01_PIN									 	GPIO_Pin_12	
#define S01_GPIO_PORT           	GPIOA
#define S01_GPIO_CLK              RCC_AHBPeriph_GPIOA
#define S01_SOURCE                GPIO_PinSource12

#define S02_PIN									 	GPIO_Pin_3
#define S02_GPIO_PORT           	GPIOB
#define S02_GPIO_CLK              RCC_AHBPeriph_GPIOB
#define S02_SOURCE                GPIO_PinSource3

#define S03_PIN									 	GPIO_Pin_4
#define S03_GPIO_PORT           	GPIOB
#define S03_GPIO_CLK              RCC_AHBPeriph_GPIOB
#define S03_SOURCE                GPIO_PinSource4

#define S04_PIN									 	GPIO_Pin_6
#define S04_GPIO_PORT           	GPIOB
#define S04_GPIO_CLK              RCC_AHBPeriph_GPIOB
#define S04_SOURCE                GPIO_PinSource6

#define S05_PIN									 	GPIO_Pin_7
#define S05_GPIO_PORT           	GPIOB
#define S05_GPIO_CLK              RCC_AHBPeriph_GPIOB
#define S05_SOURCE                GPIO_PinSource7

#define S06_PIN									 	GPIO_Pin_8
#define S06_GPIO_PORT           	GPIOB
#define S06_GPIO_CLK              RCC_AHBPeriph_GPIOB
#define S06_SOURCE                GPIO_PinSource8

/*****************************************************************
 * @brief Definition for LED pins
 *****************************************************************/ 
#define LED_RED_PIN										GPIO_Pin_11	
#define LED_RED_GPIO_PORT           	GPIOA
#define LED_RED_GPIO_CLK             	RCC_AHBPeriph_GPIOA
#define LED_RED_SOURCE               	GPIO_PinSource11
#define LED_RED_AF                   	GPIO_AF_0	 

#define LED_GREEN_PIN									GPIO_Pin_8	
#define LED_GREEN_GPIO_PORT           GPIOA
#define LED_GREEN_GPIO_CLK            RCC_AHBPeriph_GPIOA
#define LED_GREEN_SOURCE              GPIO_PinSource8
#define LED_GREEN_AF                  GPIO_AF_0	 

#define LED_BLUE_PIN									GPIO_Pin_15	
#define LED_BLUE_GPIO_PORT           	GPIOB
#define LED_BLUE_GPIO_CLK             RCC_AHBPeriph_GPIOB
#define LED_BLUE_SOURCE               GPIO_PinSource15
#define LED_BLUE_AF                   GPIO_AF_0	 
#endif
