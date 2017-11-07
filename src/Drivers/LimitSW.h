#ifndef _LIMITSW_H
#define _LIMITSW_H
	#include "stm32f0xx.h"
	#include "board.h"
	
	typedef enum {
		LIMIT_SWITCH_OK,
		LIMIT_SWITCH_LIMIT
	} LIMIT_StatusTypeDef;
	
	uint8_t LimitSW_Init(void); //Must be called from initialization section
	LIMIT_StatusTypeDef CheckLimitSwitches(void);
#endif
