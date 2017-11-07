/**
  ******************************************************************************
  * @file    USART/USART_HyperTerminalInterrupt/stm32f0xx_it.c 
  * @author  MCD Application Team
  * @version V1.4.0
  * @date    24-July-2014
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_it.h"
#include "main.h"
#include "BLDC.h"
#include "A4988.h"

/** @addtogroup STM32F0xx_StdPeriph_Examples
  * @{
  */

/** @addtogroup HyperTerminal_Interrupt
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define TXBUFFERSIZE   (countof(TxBuffer) - 1)
#define RXBUFFERSIZE   0x20

/* Private macro -------------------------------------------------------------*/
#define countof(a)   (sizeof(a) / sizeof(*(a)))

/* Private variables ---------------------------------------------------------*/
uint8_t TxBuffer[] = "\n\rUSART Hyperterminal Interrupts Example: USART-Hyperterminal\
 communication using Interrupt\n\r";
uint8_t RxBuffer[RXBUFFERSIZE];
uint8_t NbrOfDataToTransfer = TXBUFFERSIZE;
uint8_t NbrOfDataToRead = RXBUFFERSIZE;
__IO uint8_t TxCount = 0; 
__IO uint16_t RxCount = 0; 

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M0 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	SYSTICK_Callback();
}

/******************************************************************************/
/*                 STM32F0xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f0xx.s).                                               */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @brief  This function handles USART1 global interrupt request.
  * @param  None
  * @retval None
  */

void USART1_IRQHandler(void)
{
	volatile uint8_t received=0;
  if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
  {
		received=USART_ReceiveData(USART1);
		comm_receivedByte(received);
  }
}

void USART2_IRQHandler(void)
{
	volatile uint8_t received=0;	
  if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
  {
		received=USART_ReceiveData(USART2);
		comm_receivedByte(received);
  }

}
/**
  * @brief  This function handles BLDC_FG_TIMER Capture Compare interrupt request.
  * @param  None
  * @retval None
  */

void TIM14_IRQHandler(void)
{
  if(TIM_GetITStatus(STEP_PWM_TIMER, TIM_IT_Update) == SET) 
  {
    /* Clear interrupt pending bit */
    TIM_ClearITPendingBit(STEP_PWM_TIMER, TIM_IT_Update);
		
		//Process interrupt
		STEP_PWM_Completed();
	}
}
/**
  * @brief  This function handles BLDC_FG_TIMER Capture Compare and Timer Overflow interrupt request.
  * @param  None
  * @retval None
  */

void TIM17_IRQHandler(void)
{ 
  if(TIM_GetITStatus(BLDC_FG_TIMER, BLDC_FG_CC) == SET) 
  {
    /* Clear TIMx Capture compare interrupt pending bit */
    TIM_ClearITPendingBit(BLDC_FG_TIMER, BLDC_FG_CC);
		
		//Process data
		BLDC_FG_PulseDetected();
		
  }
	
  if(TIM_GetITStatus(BLDC_FG_TIMER, BLDC_FG_Update) == SET) 
  {
    /* Clear interrupt pending bit */
    TIM_ClearITPendingBit(BLDC_FG_TIMER, BLDC_FG_Update);
		/* Timer overflow - pulse is missing */
		BLDC_FG_PulseMissing(); 
		
  }
}

/**
  * @}
  */

/**
  * @}
  */

/******************************END OF FILE****/
