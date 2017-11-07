#include "LED.h"

static volatile uint8_t BlinkRequest = 0;
static volatile uint8_t BlinkCount[LED_NUMBER_OF_LEDS];

/**
  * @brief Initialize the LED driver
  * @param  None
  * @retval None
  */
uint8_t LED_Init(void){
	GPIO_InitTypeDef        GPIO_InitStructure;
	
	/* GPIOx Periph clock enable */
  RCC_AHBPeriphClockCmd(LED_RED_GPIO_CLK, ENABLE);
  RCC_AHBPeriphClockCmd(LED_GREEN_GPIO_CLK, ENABLE);
  RCC_AHBPeriphClockCmd(LED_BLUE_GPIO_CLK, ENABLE);
	
	/* Configure LED PIN as output pushpull mode */
  GPIO_InitStructure.GPIO_Pin = LED_RED_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(LED_RED_GPIO_PORT, &GPIO_InitStructure);
	
	/* Configure LED PIN as output pushpull mode */
  GPIO_InitStructure.GPIO_Pin = LED_GREEN_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(LED_GREEN_GPIO_PORT, &GPIO_InitStructure);
	
	/* Configure LED PIN as output pushpull mode */
  GPIO_InitStructure.GPIO_Pin = LED_BLUE_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(LED_BLUE_GPIO_PORT, &GPIO_InitStructure);
	return 1;
}


/**
  * @brief Blink LED "count" times
  * @param  LED - select led as LED_LedColor
  * @param  count - number of times to blink. Max 127
  * @retval 1 if OK
  */
uint8_t	LED_Blink(LED_LedColor LED, uint8_t count){
		if (count > 127) return 0;
		//Request n blinks
		BlinkCount[LED] = (count * 2) + 1; //LED should be toggled 2x times
		return 1;
}

/**
  * @brief Control Led
  * @param  LED - select led as LED_LedColor
  * @param LED_State - set LED state as LED_State
  * @retval 1 if OK
  */
uint8_t	LED_Set(LED_LedColor LED, LED_State level){
	uint32_t Pin;
	GPIO_TypeDef * Port;
	switch (LED){
		case LED_RED:
			Port = LED_RED_GPIO_PORT;
			Pin = LED_RED_PIN;
			break;
		case LED_BLUE:
			Port = LED_BLUE_GPIO_PORT;
			Pin = LED_BLUE_PIN;
		break;
		case LED_GREEN:
			Port = LED_GREEN_GPIO_PORT;
			Pin = LED_GREEN_PIN;
		break;
		default:
			return 0;
	}
	
	switch (level){
		case LED_ON:
			GPIO_SetBits(Port, Pin);
		break;
		case LED_OFF:
			GPIO_ResetBits(Port, Pin);
			//Clear corresponding bit
			BlinkRequest &= ~(1 << LED);
		break;
		case LED_SWITCH:					
			if (GPIO_ReadOutputDataBit(Port, Pin) == Bit_SET) 
					GPIO_ResetBits(Port, Pin);
			else 
					GPIO_SetBits(Port, Pin);
		break;
		case LED_BLINK:
			//Set corresponding bit
			BlinkRequest |= (1 << LED);
			break;
	}
	
	return 1;
}

/**
  * @brief Control blinking of LEDs
	*					Must be called every 100 ms
  * @retval none
  */
uint8_t	LED_Blinker(void){
	LED_LedColor LED;
	for (LED = (LED_LedColor)0; LED < LED_NUMBER_OF_LEDS; LED++){
		//Infinite blinking
		if (BlinkRequest & (1 << LED))
			LED_Set(LED, LED_SWITCH);
		
		//Blinking n counts
		if (BlinkCount[LED]){
			BlinkCount[LED]--;
			LED_Set(LED, LED_SWITCH);
			if (!BlinkCount[LED]) 
				LED_Set(LED, LED_OFF); //finally turn off LED
		}
	}
	return 1;
}
