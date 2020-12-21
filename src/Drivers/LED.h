#ifndef __LED_H
#define __LED_H

#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
	LED_ON,
	LED_OFF,
	LED_SWITCH,
	LED_BLINK,
} LED_State;

typedef enum {
	LED_RED = 0,
	LED_GREEN = 1,
	LED_BLUE = 2,
	LED_NUMBER_OF_LEDS
} LED_LedColor;

uint8_t LED_Init(void); //Must be called in initialization section
uint8_t	LED_Blinker(void); //Must be called every 100 ms
uint8_t	LED_Set(LED_LedColor LED, LED_State level);
uint8_t	LED_Blink(LED_LedColor LED, uint8_t count);
#ifdef __cplusplus
}
#endif
#endif
