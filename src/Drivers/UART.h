/* Define to prevent recursive inclusion -------------------------------------*/
//#ifndef __UART_H
//#define __UART_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx.h"
#include "board.h"
	
typedef enum 
{
  UART_USB = 0,
  UART_PCB = 1
} COM_TypeDef;   	 


void UART_COMInit(COM_TypeDef COM, USART_InitTypeDef* USART_InitStruct);	  
void UART_Init(void);
//#endif
