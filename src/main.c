/**
  ******************************************************************************
  * @file    USART/USART_Printf/main.c 
  * @author  MCD Application Team
  * @version V1.4.0
  * @date    24-July-2014
  * @brief   Main program body
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
#include "main.h"
#include "Drivers/UART.h"
#include "communication.h"
#include "Drivers/BLDC.h"
#include "Drivers/LED.h"
#include "Drivers/A4988.h"
#include "Drivers/LimitSW.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
//Timers
static volatile uint32_t Tick100ms;
static volatile uint32_t Timer100ms;
/* Private function prototypes -----------------------------------------------*/
  
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Systick interrupt handler
  */
void SYSTICK_Callback(void){
	if (++Timer100ms == 100){
		Timer100ms = 0;
		Tick100ms++;
	}
}

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{	
	//Initialize the peripherals
  UART_Init();
  BLDC_init();
	A4988_Init();
	LED_Init();
	LimitSW_Init();
	
	  /* Setup SysTick Timer for 1 msec interrupts.
     ------------------------------------------
    1. The SysTick_Config() function is a CMSIS function which configure:
       - The SysTick Reload register with value passed as function parameter.
       - Configure the SysTick IRQ priority to the lowest value (0x0F).
       - Reset the SysTick Counter register.
       - Configure the SysTick Counter clock source to be Core Clock Source (HCLK).
       - Enable the SysTick Interrupt.
       - Start the SysTick Counter.
    
    2. You can change the SysTick Clock source to be HCLK_Div8 by calling the
       SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8) just after the
       SysTick_Config() function call. The SysTick_CLKSourceConfig() is defined
       inside the misc.c file.

    3. You can change the SysTick IRQ priority by calling the
       NVIC_SetPriority(SysTick_IRQn,...) just after the SysTick_Config() function 
       call. The NVIC_SetPriority() is defined inside the core_cm0.h file.

    4. To adjust the SysTick time base, use the following formula:
                            
         Reload Value = SysTick Counter Clock (Hz) x  Desired Time base (s)
    
       - Reload Value is the parameter to be passed for SysTick_Config() function
       - Reload Value should not exceed 0xFFFFFF
   */
  if (SysTick_Config(SystemCoreClock / 1000))
  { 
    /* Capture error */ 
			printf("ERR,0");
    while (1);
  }
	
	// Turn on power LED
	LED_Set(LED_RED, LED_ON);
	
	//Main loop
  while (1)
  {
		// Parse and execute commands received over UART
		communication_callback();
							
		if (Tick100ms){
			Tick100ms--;
			//Adjust BLDC motor speed
			BLDC_RPM_control();
			//Blink LEDs
			LED_Blinker();
		}
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
	
  printf("\n\r Wrong parameter value detected on\r\n");
  printf("       file  %s\r\n", file);
  printf("       line  %d\r\n", line);
  while (1)
  {
  }
}
#endif


/*****************************END OF FILE****/
