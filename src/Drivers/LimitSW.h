#ifndef _LIMITSW_H
#define _LIMITSW_H

#include "../board.h"

#ifdef __cplusplus
extern "C" {
#endif	
	typedef enum {
		LIMIT_SWITCH_OK,
		LIMIT_SWITCH_LIMIT
	} LIMIT_StatusTypeDef;
	
	uint8_t LimitSW_Init(void); //Must be called from initialization section
	LIMIT_StatusTypeDef CheckLimitSwitches(void);
#ifdef __cplusplus
}
#endif
#endif
