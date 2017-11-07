#ifndef _A4988_H
#define _A4988_H
	#include "stm32f0xx.h"
	#include "board.h"


//-------------- STEP motor control -------------

#define STEP_PWM_CLOCK_BASE	SystemCoreClock	//FG timer base clock frequency
#define STEP_PWM_TIMER_FREQ	100000 					//TIMx PWM timer frequency
#define STEP_PWM_MIN_FREQ		2								//Minimum allowed PWM frequency. Must be higher than STEP_TIMER_FREQ / 0xFFFF
#define STEP_PWM_MAX_FREQ		50000 					//Max allowed PWM frequency. Should not exceed STEP_TIMER_FREQ / 2.
#define STEP_DEF_FREQ				200 						//Default PWM frequency
#define STEP_DEF_STEPS_PER_DIST	2400 				//Default steps per Distance value

	//Exported definitions
	typedef enum {
		STEP_NOT_MOVING = 0,
		STEP_COUNTER_CLOCKWISE = 1,
		STEP_CLOCKWISE = 2
	} STEP_DirectionTypeDef;
	
	typedef enum {
		STEP_OFF = 0,
		STEP_ON = 1
	} STEP_OnOffTypeDef;

	typedef enum {
		STEP_FULL = 0,
		STEP_HALF = 1,
		STEP_QUARTER = 2,
		STEP_EIGHTH = 3,
		STEP_SIXTEENTH	= 4,
		STEP_ERROR = 0xFF
	} STEP_MicroModeTypeDef;

	//Exported functions
	uint8_t A4988_Init(void); //Must be called from initialization section
	void STEP_PWM_Completed(void); //Must be called in corresponding Period Elapsed interrupt
	
	uint8_t STEP_MicroSet(STEP_MicroModeTypeDef	Mode);
	STEP_MicroModeTypeDef	STEP_MicroGet(void);
	uint32_t STEP_Move(uint32_t Steps);
	uint32_t STEP_MoveDist(uint32_t Dist);
	uint8_t	STEP_getStatus(STEP_DirectionTypeDef * Direction, uint32_t * Steps_moved, uint32_t * Steps_requested);
		
	uint8_t STEP_enableOn(void);
	uint8_t STEP_enableOff(void);
	STEP_OnOffTypeDef STEP_getEnable(void);
	uint8_t STEP_sleepOn(void);
	uint8_t STEP_sleepOff(void);
	STEP_OnOffTypeDef STEP_getSleep(void);
	uint8_t STEP_resetOn(void);
	uint8_t STEP_resetOff(void);
	STEP_OnOffTypeDef STEP_getReset(void);
	uint8_t STEP_setFreq(uint32_t freq);
	uint32_t STEP_getFreq(void);
	uint8_t STEP_setStepPDist(uint32_t StepPDist);
	uint32_t STEP_getStepPDist(void);
	uint8_t STEP_SetDirection(STEP_DirectionTypeDef Dir);
	STEP_DirectionTypeDef STEP_GetDirection(void);

#endif
