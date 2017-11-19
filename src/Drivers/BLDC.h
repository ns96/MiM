#ifndef _BLDC_H_
#define _BLDC_H_
	#include "stm32f0xx.h"
	#include "board.h"

//-------------- BLDC motor control -------------
//RPM
#define BLDC_RPM_MIN						300 						//min allowed RPM value
#define BLDC_RPM_MAX						15000 					//max allowed RPM value
#define BLDC_RPM_ADJUSTEMT_STEP	9								//Minimum possible motor adjusment step in RPM. Depends on max motor speed (at 100% duty) and PWM timer resolution.
//FG
#define	BLDC_FG_AVERAGING_COUNT 3 							//How many values use when averaging FGPeriod. 0 = off
#define BLDC_FG_CLOCK_BASE			SystemCoreClock //FG timer base clock frequency
#define BLDC_FG_TIMER_FREQ			100000 					//TIMx FG timer frequency
//PWM
#define BLDC_PWM_CLOCK_BASE			SystemCoreClock				//FG timer base clock frequency
#define BLDC_PWM_TIMER_FREQ			BLDC_PWM_CLOCK_BASE 	//TIMx PWM timer frequency
#define BLDC_PWM_FREQ						8000 									//Required PWM signal frequency
#define	BLDC_STARTUP_PWM				50										//PWM to be set at motor startup (0 - 100); 0 - do not use BLDC_STARTUP_PWM

//Exported definitions
typedef enum {
	BLDC_COUNTER_CLOCKWISE = 0,
	BLDC_CLOCKWISE = 1
} BLDC_DirectionTypeDef;

//Exported functions
	uint8_t BLDC_init(void); //Must be called from initialization section
	uint8_t 	BLDC_FG_PulseDetected(void); //Must be called in corresponding Input Capture interrupt
	void	BLDC_FG_PulseMissing(void); //Must be called in corresponding Timer Update interrupt
	uint8_t BLDC_RPM_control(void); //Must be called every 100 ms (or other period - depends on application)

	uint8_t BLDC_setRPM(uint32_t rpm);
	uint8_t BLDC_setPWM(uint32_t pwm);
	uint8_t BLDC_powerOn(void);
	uint8_t BLDC_powerOff(void);
	uint8_t BLDC_getPower(void);
	uint32_t BLDC_getRPM(void);
	uint8_t BLDC_getPWM(void);
	uint32_t BLDC_getFGPeriod(void);
	uint8_t BLDC_SetDirection(BLDC_DirectionTypeDef Dir);
	BLDC_DirectionTypeDef BLDC_GetDirection(void);
#endif

