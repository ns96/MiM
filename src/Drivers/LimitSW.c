#include "LimitSW.h"
/**
  * @brief Initialize the limit switch driver
  * @param  None
  * @retval None
  */
	

//---------------  Internal macros -------------------
#define STEP_LimitSwitch1_Get()	GPIO_ReadInputDataBit(S01_GPIO_PORT, S01_PIN)
#define STEP_LimitSwitch2_Get()	GPIO_ReadInputDataBit(S02_GPIO_PORT, S02_PIN)
#define STEP_LIMIT_ON			Bit_RESET
#define STEP_LIMIT_OFF		Bit_SET

uint8_t LimitSW_Init(void){
	GPIO_InitTypeDef        GPIO_InitStructure;
	
	/* GPIOx Periph clock enable */
  RCC_AHBPeriphClockCmd(S01_GPIO_CLK, ENABLE);
  RCC_AHBPeriphClockCmd(S02_GPIO_CLK, ENABLE);
	
	/* Configure S0x PIN as input mode */
  GPIO_InitStructure.GPIO_Pin = S01_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(S01_GPIO_PORT, &GPIO_InitStructure);
	
	/* Configure S0x PIN as input mode */
  GPIO_InitStructure.GPIO_Pin = S02_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(S02_GPIO_PORT, &GPIO_InitStructure);
	
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
