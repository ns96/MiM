#include <Arduino.h>
#include <stdint.h>
#include "LimitSW.h"

//---------------  Internal macros -------------------
#define STEP_LimitSwitch1_Get()	digitalReadNoPWM(S01_PIN)
#define STEP_LimitSwitch2_Get()	digitalReadNoPWM(S02_PIN)
#define STEP_LIMIT_ON			0
#define STEP_LIMIT_OFF			1

/**
 * Read state of digital PIN without turning off PWM.
 * Original Arduino function also disables PWM on the input pin that affects timers.
 * @param pin digital pin to read state
 * @return HIGH or LOW
 */
PinStatus digitalReadNoPWM(uint8_t pin)
{
	/* Get bit mask and check valid pin */
	uint8_t bit_mask = digitalPinToBitMask(pin);
	if (bit_mask == NOT_A_PIN || isDoubleBondedActive(pin)) {
	    return LOW;
	}
	
	/* Get port and check valid port */
	PORT_t *port = digitalPinToPortStruct(pin);

	/* Read pin value from PORTx.IN register */
	if (port->IN & bit_mask){
		return HIGH;
	} else {
		return LOW;
	}
}


uint8_t LimitSW_Init(void){
    pinMode(S01_PIN, INPUT_PULLUP);
    pinMode(S02_PIN, INPUT_PULLUP);
	
	return 1;
}

/**
  * @brief  Check state of limit switches
  * @param  None
  * @retval Returns LIMIT_SWITCH_LIMIT if one or both switches are pressed
  */
LIMIT_StatusTypeDef CheckLimitSwitches(void){
	if((STEP_LimitSwitch1_Get() == STEP_LIMIT_ON) || (STEP_LimitSwitch2_Get() == STEP_LIMIT_ON)){
		return LIMIT_SWITCH_LIMIT;
	}
	return LIMIT_SWITCH_OK;
}
