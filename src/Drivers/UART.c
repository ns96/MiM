#include "main.h"
#include "UART.h"
/** @addtogroup USART_Printf
  * @{
  */ 
	
USART_TypeDef* COM_USART[] = {EVAL_COM1_UART_USB,EVAL_COM2_UART_PCB}; 
GPIO_TypeDef* COM_TX_PORT[] = {EVAL_COM1_TX_GPIO_PORT,EVAL_COM2_TX_GPIO_PORT};
GPIO_TypeDef* COM_RX_PORT[] = {EVAL_COM1_RX_GPIO_PORT,EVAL_COM2_RX_GPIO_PORT};
const uint32_t COM_USART_CLK[] = {EVAL_COM1_CLK,EVAL_COM2_CLK};
const uint32_t COM_TX_PORT_CLK[] = {EVAL_COM1_TX_GPIO_CLK,EVAL_COM2_TX_GPIO_CLK};
const uint32_t COM_RX_PORT_CLK[] = {EVAL_COM1_RX_GPIO_CLK,EVAL_COM2_RX_GPIO_CLK};
const uint16_t COM_TX_PIN[] = {EVAL_COM1_TX_PIN,EVAL_COM2_TX_PIN};
const uint16_t COM_RX_PIN[] = {EVAL_COM1_RX_PIN,EVAL_COM2_RX_PIN};
const uint8_t COM_TX_PIN_SOURCE[] = {EVAL_COM1_TX_SOURCE,EVAL_COM2_TX_SOURCE};
const uint8_t COM_RX_PIN_SOURCE[] = {EVAL_COM1_RX_SOURCE,EVAL_COM2_RX_SOURCE};
const uint8_t COM_TX_AF[] = {EVAL_COM1_TX_AF,EVAL_COM2_TX_AF};
const uint8_t COM_RX_AF[] = {EVAL_COM1_RX_AF,EVAL_COM2_RX_AF};
const uint8_t COM_RX_IRQn[] = {EVAL_COM1_IRQn,EVAL_COM2_IRQn};
static void USART_Config(void);

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
	
/**
  * @brief Initialize the UART driver
  * @param  None
  * @retval None
  */
void UART_Init(void){
	USART_Config();
}

/**
  * @brief Configure the USART Device
  * @param  None
  * @retval None
  */
static void USART_Config(void)
{ 
  USART_InitTypeDef USART_InitStructure;
  
  /* USARTx configured as follow:
  - BaudRate = 115200 baud  
  - Word Length = 8 Bits
  - Stop Bit = 1 Stop Bit
  - Parity = No Parity
  - Hardware flow control disabled (RTS and CTS signals)
  - Receive and transmit enabled
  */
  USART_InitStructure.USART_BaudRate = 19200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  
  UART_COMInit(UART_USB, &USART_InitStructure);
	//UART_COMInit(UART_PCB, &USART_InitStructure);
	

}

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART */
  USART_SendData(EVAL_COM1_UART_USB, (uint8_t) ch);
	USART_SendData(EVAL_COM2_UART_PCB, (uint8_t) ch);
  /* Loop until transmit data register is empty */
  while (USART_GetFlagStatus(EVAL_COM1_UART_USB, USART_FLAG_TXE) == RESET)
  {}
  /* Loop until transmit data register is empty */
  while (USART_GetFlagStatus(EVAL_COM2_UART_PCB, USART_FLAG_TXE) == RESET)
  {}
  return ch;
}

void UART_COMInit(COM_TypeDef COM, USART_InitTypeDef* USART_InitStruct)
{
  GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

  /* Enable GPIO clock */
  RCC_AHBPeriphClockCmd(COM_TX_PORT_CLK[COM] | COM_RX_PORT_CLK[COM], ENABLE);

  /* Enable USART clock */
  RCC_APB2PeriphClockCmd(COM_USART_CLK[COM], ENABLE); 
  /* Enable USART clock */
  RCC_APB1PeriphClockCmd(COM_USART_CLK[COM], ENABLE); 
  /* Connect PXx to USARTx_Tx */
  GPIO_PinAFConfig(COM_TX_PORT[COM], COM_TX_PIN_SOURCE[COM], COM_TX_AF[COM]);

  /* Connect PXx to USARTx_Rx */
  GPIO_PinAFConfig(COM_RX_PORT[COM], COM_RX_PIN_SOURCE[COM], COM_RX_AF[COM]);
  
  /* Configure USART Tx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = COM_TX_PIN[COM];
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(COM_TX_PORT[COM], &GPIO_InitStructure);
    
  /* Configure USART Rx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = COM_RX_PIN[COM];
  GPIO_Init(COM_RX_PORT[COM], &GPIO_InitStructure);

  /* USART configuration */
  USART_Init(COM_USART[COM], USART_InitStruct);
    	
 /* NVIC configuration */
  /* Enable the USARTx Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = COM_RX_IRQn[COM];
  NVIC_InitStructure.NVIC_IRQChannelPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
	USART_ITConfig(COM_USART[COM], USART_IT_RXNE, ENABLE);	
	
  /* Enable USART */
  USART_Cmd(COM_USART[COM], ENABLE);
	
}
