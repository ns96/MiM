#include <Arduino.h>
#include "LimitSW.h"
/**
  * @brief Initialize the limit switch driver
  * @param  None
  * @retval None
  */

//---------------  Internal macros -------------------
#define STEP_LimitSwitch1_Get()	digitalRead(S01_PIN)
#define STEP_LimitSwitch2_Get()	digitalRead(S02_PIN)
#define STEP_LIMIT_ON			0
#define STEP_LIMIT_OFF			1

uint8_t LimitSW_Init(void){
  pinMode(S01_PIN, INPUT);
  pinMode(S02_PIN, INPUT);
	
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
