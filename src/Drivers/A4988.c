#include <stdint.h>
#include "A4988.h"
#include "LED.h"
#include "LimitSW.h"

//-------------- private variables -------------
//static TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
//static TIM_OCInitTypeDef  			TIM_OCInitStructure;
static volatile uint32_t	STEP_steps_requested = 0, STEP_steps_moved = 0;	//Steps requested and moved
static volatile uint32_t	STEP_PWM_freq = STEP_DEF_FREQ, STEP_PWM_Actual_freq = 0;	//Motor PWM frequency
static volatile uint32_t	STEP_StepPDist = STEP_DEF_STEPS_PER_DIST;	//Steps per Distes value
static volatile STEP_MicroModeTypeDef	STEP_MicroStepMode = STEP_FULL;

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
void STEP_PWM_Completed(void){
	//If limit swithes are not pressed and more steps are requested
	if ((CheckLimitSwitches() == LIMIT_SWITCH_OK) && (STEP_steps_requested - STEP_steps_moved)){			
			//Turn on LED
			LED_Set(LED_GREEN, LED_BLINK);
		
			/* Different PWM frequency was requested
			*	 Change it before generating another step
			*/
			if (STEP_PWM_Actual_freq != STEP_PWM_freq){
				STEP_Set_Timer_PWM(STEP_PWM_freq);
			} 
			//Just generate another step
			else {				
//				TIM_Cmd(STEP_PWM_TIMER, ENABLE);  //Turn on timer
			}
		
			STEP_steps_moved++;
	} else {
		STEP_Stop();
	}
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
	//Switch mode
	switch(Mode){
		case STEP_FULL:

			break;
		case STEP_HALF:

			break;
		case STEP_QUARTER:

			break;
		case STEP_EIGHTH:

			break;
		case STEP_SIXTEENTH:

			break;
	}
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
//	GPIO_ResetBits(STEP_ENABLE_GPIO_PORT,STEP_ENABLE_PIN);
	return 1;
}

/**
 * \brief Turns off enable PIN for STEP motor
 */
uint8_t STEP_enableOff(void){
	STEP_Stop();
//	GPIO_SetBits(STEP_ENABLE_GPIO_PORT,STEP_ENABLE_PIN);
	return 1;
}

/**
 * \brief Get status of STEP motor enable
 */
STEP_OnOffTypeDef STEP_getEnable(void){
//	if (GPIO_ReadOutputDataBit(STEP_ENABLE_GPIO_PORT, STEP_ENABLE_PIN) == Bit_SET) 
//		return STEP_OFF;
//	else return STEP_ON;
}

/**
 * \brief Turns on sleep PIN for STEP motor
 */
uint8_t STEP_sleepOn(void){
	STEP_Stop();
//	GPIO_ResetBits(STEP_SLEEP_GPIO_PORT,STEP_SLEEP_PIN);
	return 1;
}

/**
 * \brief Turns off sleep PIN for STEP motor
 */
uint8_t STEP_sleepOff(void){
	STEP_Stop(); //clear previously requested steps if there are any
//	GPIO_SetBits(STEP_SLEEP_GPIO_PORT,STEP_SLEEP_PIN);
	return 1;
}

/**
 * \brief Get status of STEP motor sleep
 */
STEP_OnOffTypeDef STEP_getSleep(void){
//	if (GPIO_ReadOutputDataBit(STEP_SLEEP_GPIO_PORT, STEP_SLEEP_PIN) == Bit_SET) 
		return STEP_OFF;
//	else return STEP_ON;
}

/**
 * \brief Turns on reset PIN for STEP motor
 */
uint8_t STEP_resetOn(void){
	STEP_Stop();
//	GPIO_ResetBits(STEP_RESET_GPIO_PORT,STEP_RESET_PIN);
	return 1;
}

/**
 * \brief Turns off reset PIN for STEP motor
 */
uint8_t STEP_resetOff(void){
	STEP_Stop(); //clear previously requested steps if there are any
//	GPIO_SetBits(STEP_RESET_GPIO_PORT,STEP_RESET_PIN);
	return 1;
}

/**
 * \brief Get status of STEP motor reset
 */
STEP_OnOffTypeDef STEP_getReset(void){
//	if (GPIO_ReadOutputDataBit(STEP_RESET_GPIO_PORT, STEP_RESET_PIN) == Bit_SET) 
		return STEP_OFF;
//	else return STEP_ON;
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
//			GPIO_SetBits(STEP_DIR_GPIO_PORT, STEP_DIR_PIN);
			return 1;
		case STEP_COUNTER_CLOCKWISE:
//			GPIO_ResetBits(STEP_DIR_GPIO_PORT, STEP_DIR_PIN);
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
//	if (GPIO_ReadOutputDataBit(STEP_DIR_GPIO_PORT, STEP_DIR_PIN) == Bit_SET) 
		return STEP_CLOCKWISE;
//	else
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
		//Calculate new period
		uint32_t NewPeriod = STEP_PWM_TIMER_FREQ / freq;
	
		//Store actual PWM frequency
		STEP_PWM_Actual_freq = freq;
		
		//Set PWM frequency with 50% duty
//		TIM_TimeBaseStructure.TIM_Period = (uint16_t) NewPeriod; // set period duration
//		TIM_TimeBaseInit(STEP_PWM_TIMER, &TIM_TimeBaseStructure);// reinititialise timer with new period value
//		TIM_OCInitStructure.TIM_Pulse = (uint16_t) (NewPeriod / 2); // set pulse duration (50% Duty)
//		TIM_OC1Init(STEP_PWM_TIMER, &TIM_OCInitStructure); //reinititialise output compare register
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
#if 0
	GPIO_InitTypeDef        GPIO_InitStructure;
	
	/* GPIOx Periph clock enable */
  RCC_AHBPeriphClockCmd(STEP_ENABLE_GPIO_CLK, ENABLE);
  RCC_AHBPeriphClockCmd(STEP_SLEEP_GPIO_CLK, ENABLE);
  RCC_AHBPeriphClockCmd(STEP_RESET_GPIO_CLK, ENABLE);
  RCC_AHBPeriphClockCmd(STEP_PWM_GPIO_CLK, ENABLE);
  RCC_AHBPeriphClockCmd(STEP_DIR_GPIO_CLK, ENABLE);
  RCC_AHBPeriphClockCmd(STEP_MS1_GPIO_CLK, ENABLE);
  RCC_AHBPeriphClockCmd(STEP_MS2_GPIO_CLK, ENABLE);
  RCC_AHBPeriphClockCmd(STEP_MS3_GPIO_CLK, ENABLE);
	
	/* Configure ENABLE PIN as output pushpull mode */
  GPIO_InitStructure.GPIO_Pin = STEP_ENABLE_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(STEP_ENABLE_GPIO_PORT, &GPIO_InitStructure);
	
	/* Configure SLEEP PIN as output pushpull mode */
  GPIO_InitStructure.GPIO_Pin = STEP_SLEEP_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(STEP_SLEEP_GPIO_PORT, &GPIO_InitStructure);
	
	/* Configure RESET PIN as output pushpull mode */
  GPIO_InitStructure.GPIO_Pin = STEP_RESET_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(STEP_RESET_GPIO_PORT, &GPIO_InitStructure);
	
	/* Configure DIR PIN as output pushpull mode */
  GPIO_InitStructure.GPIO_Pin = STEP_DIR_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(STEP_DIR_GPIO_PORT, &GPIO_InitStructure);
		
  /* GPIOA Configuration: PWM out - STEP_PWM_TIMER CH2 */
  GPIO_InitStructure.GPIO_Pin = STEP_PWM_PIN ;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(STEP_PWM_GPIO_PORT, &GPIO_InitStructure); 
	
	/* Configure MSx as output pushpull mode */
  GPIO_InitStructure.GPIO_Pin = STEP_MS1_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(STEP_MS1_GPIO_PORT, &GPIO_InitStructure);
	
	/* Configure MSx as output pushpull mode */
  GPIO_InitStructure.GPIO_Pin = STEP_MS2_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(STEP_MS2_GPIO_PORT, &GPIO_InitStructure);
    
	/* Configure MSx as output pushpull mode */
  GPIO_InitStructure.GPIO_Pin = STEP_MS3_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(STEP_MS3_GPIO_PORT, &GPIO_InitStructure);
	
  /* Connect TIM Channels to AF2 */
  GPIO_PinAFConfig(STEP_PWM_GPIO_PORT, STEP_PWM_SOURCE, STEP_PWM_AF);
 #endif
}

/**
  * @brief  Configure the motor PWM TIMER and pins.
  * @param  None
  * @retval None
  */
static void STEP_PWM_Config(void)
{
#if 0
  NVIC_InitTypeDef NVIC_InitStructure;
	
  /* PWM_PWM_TIMER clock enable */
  RCC_APB1PeriphClockCmd(STEP_PWM_TIMER_CLK, ENABLE);
 
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
  TIM_OCStructInit(&TIM_OCInitStructure);

  /* ---------------------------------------------------------------------------
    Note: 
     SystemCoreClock variable holds HCLK frequency and is defined in system_stm32f0xx.c file.
     Each time the core clock (HCLK) changes, user had to call SystemCoreClockUpdate()
     function to update SystemCoreClock variable value. Otherwise, any configuration
     based on this variable will be incorrect. 
     
  --------------------------------------------------------------------------- */
  	
  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = (uint16_t)(STEP_PWM_TIMER_FREQ / STEP_PWM_freq);
  TIM_TimeBaseStructure.TIM_Prescaler = (uint16_t)((STEP_PWM_CLOCK_BASE / STEP_PWM_TIMER_FREQ) - 1); 
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(STEP_PWM_TIMER, &TIM_TimeBaseStructure);

  /* Output Compare Active Mode configuration: Channel2 */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
  TIM_OCInitStructure.TIM_Pulse = (uint16_t)((STEP_PWM_TIMER_FREQ / STEP_PWM_freq) / 2); //Set Pulse to 50% of period
  TIM_OC1Init(STEP_PWM_TIMER, &TIM_OCInitStructure);
		
  TIM_ARRPreloadConfig(STEP_PWM_TIMER, DISABLE); 
  TIM_OC1PreloadConfig(STEP_PWM_TIMER, TIM_OCPreload_Disable);

  /* Enable the TIMx global Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = STEP_PWM_TIM_IRQ;
  NVIC_InitStructure.NVIC_IRQChannelPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
	//Set OnePulse Mode On
	TIM_SelectOnePulseMode(STEP_PWM_TIMER, TIM_OPMode_Single); 
	
	//Enable Step completed interrupt
	TIM_ITConfig(STEP_PWM_TIMER, TIM_IT_Update, ENABLE); 
#endif
}
