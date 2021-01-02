#include <Arduino.h>
#include <stdint.h>
#include "A4988.h"
#include "LED.h"
#include "LimitSW.h"

//-------------- private variables -------------
static volatile uint32_t	STEP_steps_requested = 0, STEP_steps_moved = 0;	//Steps requested and moved
static volatile uint32_t	STEP_PWM_freq = STEP_DEF_FREQ, STEP_PWM_Actual_freq = 0;	//Motor PWM frequency
static volatile uint32_t	STEP_StepPDist = STEP_DEF_STEPS_PER_DIST;	//Steps per Distes value
static volatile STEP_MicroModeTypeDef	STEP_MicroStepMode = STEP_FULL;

static volatile uint8_t STEP_reset = 0, STEP_sleep = 0;


//---------------  Internal functions prototypes -------------------
static void STEP_GPIO_Config(void);
static uint8_t STEP_Set_Timer_PWM(uint32_t freq);
static void STEP_Stop(void);
static uint8_t STEP_MicroStepMode2Multiplier(STEP_MicroModeTypeDef STEP_MicroStepMode);
static void STEP_PWM_Config(void);

/** *****************************************
*				Exported Functions - configuration							
****************************************** */

/**
  * @brief Initialize the A4988 driver
  * @param  None
  * @retval None
  */
uint8_t A4988_Init(void)
{	
	STEP_GPIO_Config();
	STEP_enableOff();
	STEP_SetDirection(STEP_CLOCKWISE);
	STEP_sleepOff();
	STEP_resetOff();
	STEP_PWM_Config();
	
	return 1;
}


/** ********************************************************
*				Exported Functions - interrupts and timers handling							
********************************************************* */

/**
 * \brief One STEP pulse comleted, make one more step if required
 * 				Must be called in corresponding Period Elapsed interrupt
 */
//void STEP_PWM_Completed(void){
ISR(TCA0_CMP0_vect) {

	//If limit swithes are not pressed and more steps are requested
	if ((CheckLimitSwitches() == LIMIT_SWITCH_OK) && (STEP_steps_requested - STEP_steps_moved - 1)){			
			//Turn on LED
			LED_Set(LED_GREEN, LED_BLINK);
			/* Different PWM frequency was requested
			*	 Change it before generating another step
			*/
			if (STEP_PWM_Actual_freq != STEP_PWM_freq){
				STEP_Set_Timer_PWM(STEP_PWM_freq);
			}
			STEP_steps_moved++;
			
	} else {
		STEP_Stop();
	}
	/* The interrupt flag has to be cleared manually */
	TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm;
}

/** *****************************************
*				Exported Functions - operation
****************************************** */

/**
 * \brief Move motor by steps
 * \param Steps - number of steps to move (up to 32 bit value)
 * \retval Returns steps to move
 */
uint32_t STEP_Move(uint32_t Steps){
	//Exit if limit swithes are pressed
	if (CheckLimitSwitches() != LIMIT_SWITCH_OK) return 0;
	
	//Exit if motor is not in required state
	if ((STEP_getEnable() == STEP_OFF) || (STEP_getSleep() == STEP_ON) || (STEP_getReset() == STEP_ON)) return 0;
	
	//Start moving only if parameters are set
	if ((Steps != 0) && (STEP_PWM_freq != 0)) 
	{						
		//request Steps
		STEP_steps_requested = Steps;	
		STEP_steps_moved = 0;
		
		//Set desired frequency
		STEP_Set_Timer_PWM(STEP_PWM_freq);
			
	} else {
		STEP_Stop(); //Stop moving motor
		return 0;
	}
	return Steps;	
}

/**
 * \brief Move motor by distance
 * \retval steps to move
 */
uint32_t STEP_MoveDist(uint32_t Dist)
{	
	uint8_t Multiplier = STEP_MicroStepMode2Multiplier(STEP_MicroStepMode);	
	uint64_t StepsToMove = Dist * STEP_StepPDist * Multiplier;
	if (StepsToMove > 0xFFFFFFFF) return 0; //Cancel if requested value is too high
	return STEP_Move((uint32_t)StepsToMove);
}

/**
 * \brief Get moving status of motor
 * \param Move - pointer to Motor Moving variable
 * \param Direction - pointer to Motor Direction variable
 * \param STEP_steps_moved - pointer to Moved Steps variable
 * \param STEP_steps_requested - pointer to Requested Steps variable
 */
uint8_t	STEP_getStatus(STEP_DirectionTypeDef * Direction, uint32_t * Steps_moved, uint32_t * Steps_requested){
	if (STEP_steps_requested - STEP_steps_moved) {
		*Direction = STEP_GetDirection();
	} else *Direction = STEP_NOT_MOVING;
	*Steps_moved = STEP_steps_moved;
	*Steps_requested = STEP_steps_requested;
	return 1;
}

/**
 * \brief Set microstepping mode
 * 
 */
uint8_t STEP_MicroSet(STEP_MicroModeTypeDef	Mode){
	//Save current mode
	STEP_MicroStepMode = Mode;
	return 1;
}
/**
 * \brief Get microstepping mode
 */
STEP_MicroModeTypeDef	STEP_MicroGet(void){

	return STEP_MicroStepMode;
}
	
/**
 * \brief Turns on enable PIN for STEP motor
 */
uint8_t STEP_enableOn(void){
	STEP_Stop(); //clear previously requested steps if there are any
	STEP_GPIO_Config();
	digitalWrite(STEP_ENABLE_PIN,LOW);
	STEP_PWM_Config();
	return 1;
}

/**
 * \brief Turns off enable PIN for STEP motor
 */
uint8_t STEP_enableOff(void){
	STEP_Stop();
	digitalWrite(STEP_ENABLE_PIN,HIGH);
						
	TCA0.SINGLE.INTCTRL = 0 << TCA_SINGLE_CMP0_bp   /* Compare 0 Interrupt: disabled */
                     | 0 << TCA_SINGLE_CMP1_bp /* Compare 1 Interrupt: disabled */
                     | 0 << TCA_SINGLE_CMP2_bp /* Compare 2 Interrupt: disabled */
                     | 0 << TCA_SINGLE_OVF_bp; /* Overflow Interrupt: enabled */
	return 1;
}

/**
 * \brief Get status of STEP motor enable
 */
STEP_OnOffTypeDef STEP_getEnable(void){
	if (digitalRead(STEP_ENABLE_PIN) == Bit_SET) 
		return STEP_OFF;
	else return STEP_ON;
}

/**
 * \brief Turns on sleep PIN for STEP motor
 */
uint8_t STEP_sleepOn(void){
	STEP_Stop();
	STEP_sleep = 0;
//	GPIO_ResetBits(STEP_SLEEP_GPIO_PORT,STEP_SLEEP_PIN);
	return 1;
}

/**
 * \brief Turns off sleep PIN for STEP motor
 */
uint8_t STEP_sleepOff(void){
	STEP_Stop(); //clear previously requested steps if there are any
	STEP_sleep = 1;
//	GPIO_SetBits(STEP_SLEEP_GPIO_PORT,STEP_SLEEP_PIN);
	return 1;
}

/**
 * \brief Get status of STEP motor sleep
 */
STEP_OnOffTypeDef STEP_getSleep(void){
	if (STEP_sleep) 
		return STEP_OFF;
	else 
		return STEP_ON;
}

/**
 * \brief Turns on reset PIN for STEP motor
 */
uint8_t STEP_resetOn(void){
	STEP_Stop();
	STEP_reset = 0;
//	GPIO_ResetBits(STEP_RESET_GPIO_PORT,STEP_RESET_PIN);
	return 1;
}

/**
 * \brief Turns off reset PIN for STEP motor
 */
uint8_t STEP_resetOff(void){
	STEP_Stop(); //clear previously requested steps if there are any
	STEP_reset = 1;
//	GPIO_SetBits(STEP_RESET_GPIO_PORT,STEP_RESET_PIN);
	return 1;
}

/**
 * \brief Get status of STEP motor reset
 */
STEP_OnOffTypeDef STEP_getReset(void){
	if (STEP_reset) 
		return STEP_OFF;
	else 
		return STEP_ON;
}

/**
 * \brief Sets STEP motor PWM frequency
 * \param freq - PWM frequency
 */
uint8_t STEP_setFreq(uint32_t freq)
{	
	STEP_PWM_freq = freq;
	return 1;
}

/**
 * \brief Gets STEP motor PWM frequency
 * \return freq - PWM frequency
 */
uint32_t STEP_getFreq(void)
{	
	return STEP_PWM_freq;
}

/**
 * \brief Sets Steps per Distance value
 * \param STEP_StepPDist - Steps per Distance
 */
uint8_t STEP_setStepPDist(uint32_t StepPDist)
{	
	STEP_StepPDist = StepPDist;
	return 1;
}

/**
 * \brief Gets Steps per Distance value
 * \return STEP_StepPDist - Steps per Distance
 */
uint32_t STEP_getStepPDist(void)
{	
	return STEP_StepPDist;
}

/**
 * \brief Set direction of STEP motor
 * \retval 1 if OK
 */
uint8_t STEP_SetDirection(STEP_DirectionTypeDef Dir)
{
	switch (Dir){
		case STEP_CLOCKWISE:
			digitalWrite(STEP_DIR_PIN,HIGH);
			return 1;
		case STEP_COUNTER_CLOCKWISE:
			digitalWrite(STEP_DIR_PIN,LOW);
			return 1;
		default:
			return 0;
	}
}

/**
 * \brief Get direction of STEP motor
 * \retval Direction in STEP_DirectionTypeDef type
 */
STEP_DirectionTypeDef STEP_GetDirection(void){
	if (digitalRead(STEP_DIR_PIN) == Bit_SET) 
		return STEP_CLOCKWISE;
	else
		return STEP_COUNTER_CLOCKWISE;
}

/** *****************************************
*				Internal Functions									
****************************************** */

/**
  * @brief  Set PWM Timer parameters to generate required frequency
  * @param  freq - required PWM frequency
  * @retval Status - 1 if OK
  */
static uint8_t STEP_Set_Timer_PWM(uint32_t freq){
		//Store actual PWM frequency
		STEP_PWM_Actual_freq = freq;
		
		//Calculate new period
		uint32_t STEP_NewPeriod = STEP_PWM_TIMER_FREQ / freq;

		//Set PWM frequency with 50% duty
		//refresh frequency when pin is low (phase 2)
		TCA0.SINGLE.CMP0BUF = (uint16_t)(STEP_NewPeriod / 2); /* Compare Register 1: 0x10 */
		TCA0.SINGLE.PERBUF = (uint16_t)(STEP_NewPeriod); /* Period*/
		//enable output if disabled
		if ((TCA0.SINGLE.CTRLB & (1 << TCA_SINGLE_CMP0EN_bp)) == 0){
			TCA0.SINGLE.CTRLB |= (1 << TCA_SINGLE_CMP0EN_bp);
		}
		
		return 1;
}
/**
  * @brief  Stop moving motor
  * @param  None
  * @retval None
  */
static void STEP_Stop(void){
		//Cancel request
		STEP_steps_requested = 0;
		STEP_steps_moved = 0;
		//disable output
		TCA0.SINGLE.CTRLB &= ~(1 << TCA_SINGLE_CMP0EN_bp);
		//Turn off LED
		LED_Set(LED_GREEN, LED_OFF);
}

/**
  * @brief  Convert MicroStepMode to steps multiplier
  * @param  MicroSteppping Mode
  * @retval Multiplier of steps
  */
static uint8_t STEP_MicroStepMode2Multiplier(STEP_MicroModeTypeDef MicroStepMode){
		switch (MicroStepMode){
		case STEP_FULL:
			return 1;
		case STEP_HALF:
			return 2;
		case STEP_QUARTER:
			return 4;
		case STEP_EIGHTH:
			return 8;
		case STEP_SIXTEENTH:
			return 16;
		}
		
		return 0; //Error
}


/**
  * @brief  Configure the GPIO.
  * @param  None
  * @retval None
  */
static void STEP_GPIO_Config(void)
{
	pinMode(STEP_ENABLE_PIN, OUTPUT);	
	pinMode(STEP_DIR_PIN, OUTPUT);
	pinMode(STEP_PWM_PIN, OUTPUT);
	digitalWrite(STEP_PWM_PIN, LOW);
	PORTMUX.TCAROUTEA = STEP_PWM_PORTMUX;
}

/**
  * @brief  Configure the motor PWM TIMER and pins.
  * @param  None
  * @retval None
  */
static void STEP_PWM_Config(void)
{	
	uint32_t STEP_NewPeriod = STEP_PWM_TIMER_FREQ / STEP_PWM_freq;
	STEP_PWM_Actual_freq = STEP_PWM_freq;

	TCA0.SINGLE.CTRLB = 0 << TCA_SINGLE_ALUPD_bp         /* Auto Lock Update: disabled */
	                    | 0 << TCA_SINGLE_CMP0EN_bp      /* Compare 0 Enable: disabled */
	                    | 0 << TCA_SINGLE_CMP1EN_bp      /* Compare 1 Enable: disabled */
	                    | 0 << TCA_SINGLE_CMP2EN_bp      /* Compare 2 Enable: disabled */ 
						| TCA_SINGLE_WGMODE_SINGLESLOPE_gc; /*  */
 
	TCA0.SINGLE.CMP0BUF = (uint16_t)(STEP_NewPeriod / 2); /* Compare Register 1: 0x10 */
	TCA0.SINGLE.PERBUF = (uint16_t)(STEP_NewPeriod); /* Period*/

	TCA0.SINGLE.INTCTRL = 1 << TCA_SINGLE_CMP0_bp   /* Compare 0 Interrupt: enabled */
                     | 0 << TCA_SINGLE_CMP1_bp /* Compare 1 Interrupt: disabled */
                     | 0 << TCA_SINGLE_CMP2_bp /* Compare 2 Interrupt: disabled */
                     | 0 << TCA_SINGLE_OVF_bp; /* Overflow Interrupt: disabled */

                    
	TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV2_gc /* System Clock / 2 */
                      | 1 << TCA_SINGLE_ENABLE_bp /* Module Enable: enabled */;
					  
}
