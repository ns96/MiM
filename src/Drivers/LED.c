#include <Arduino.h>
#include <stdint.h>
#include "LED.h"
#include  "../board.h"
static volatile uint8_t BlinkRequest = 0;
static volatile uint8_t BlinkCount[LED_NUMBER_OF_LEDS];

/**
  * @brief Initialize the LED driver
  * @param  None
  * @retval None
  */
uint8_t LED_Init(void){
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
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
  int pin;
	switch (LED){
		case LED_RED:
			pin = LED_RED_PIN; 
		break;
		case LED_BLUE:
			pin = LED_BLUE_PIN; 
		break;
		case LED_GREEN:
			pin = LED_GREEN_PIN;
		break;
		default:
			return 0;
	}
	
	switch (level){
		case LED_ON:
			digitalWrite(pin, HIGH);
		break;
		case LED_OFF:
			digitalWrite(pin, LOW);
			//Clear corresponding bit
			BlinkRequest &= ~(1 << LED);
		break;
		case LED_SWITCH:					
			if (digitalRead(pin)) 
					digitalWrite(pin, LOW);
			else 
					digitalWrite(pin, HIGH);
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
	for (int i = 0; i < LED_NUMBER_OF_LEDS; i++){
		//Infinite blinking
		if (BlinkRequest & (1 << i))
			LED_Set(i, LED_SWITCH);
		
		//Blinking n counts
		if (BlinkCount[i]){
			BlinkCount[i]--;
			LED_Set(i, LED_SWITCH);
			if (!BlinkCount[i]) 
				LED_Set(i, LED_OFF); //finally turn off LED
		}
	}
	return 1;
}
