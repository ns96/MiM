#include "BLDC.h"
#include "main.h"
#include "LED.h"
#include <stdbool.h>
#include <stdlib.h>

//-------------- private variables -------------
	GPIO_InitTypeDef        GPIO_InitStructure;
	TIM_OCInitTypeDef  			TIM_OCInitStructure;
	static volatile bool RPM_Adjust_requested = false, BLDC_Startup_Mode = false;
	static volatile uint32_t BLDC_RPM_target = 0;
	static volatile uint16_t FGPeriod = 0; //Period of FG signal
	static volatile uint32_t motorHalted_cnt=0;
	//If FGPeriod averaging is going to be used
	#if BLDC_FG_AVERAGING_COUNT != 0
	static uint8_t  FGPeriodHistInd = 0, FGPeriodHistCnt = 0; //History of pulse length required for averaging: index in array and registered pulses count
	static uint16_t FGPeriodHist[BLDC_FG_AVERAGING_COUNT]; //History of pulse length required for averaging
	#endif

//---------------  Internal functions prototypes -------------------
	static inline uint16_t FG2RPM (uint16_t FGPeriod);
	static inline uint32_t  RPM2PWM(uint32_t rpm);
	static void BLDC_GPIO_Config(void);
	static void TIM_FG_Config(void);
	static void TIM_PWM_Config(void);

/** *****************************************
*				Exported Functions - configuration							
****************************************** */

/**
 * \brief Initializes the BLDC motor driver
 *
 */
uint8_t BLDC_init(void){
	BLDC_GPIO_Config();
	TIM_PWM_Config();
	TIM_FG_Config();
	BLDC_SetDirection(BLDC_CLOCKWISE);
	BLDC_powerOff();
	BLDC_RPM_target=0;
	return 1;
}

/** ********************************************************
*				Exported Functions - interrupts and timers handling							
********************************************************* */

/**
 * \brief FG period measuring timer has timed out. FG pulse is missing.
 * 				Must be called in corresponding Timer Update interrupt
 */
void BLDC_FG_PulseMissing(void){
	//Stop blinking LED
	LED_Set(LED_GREEN, LED_OFF);
	//Is the motor supposed to be running?
	if (TIM_GetCapture2(BLDC_PWM_TIMER)>0){
		//Increment motor halted timer. 
		//No signs of motor spinning for ~10seconds?
		if (++motorHalted_cnt>10){
			//Set motor control values to defaults
			BLDC_setPWM(0);
			BLDC_RPM_target=0;
			//Turn off power for motor
			BLDC_powerOff();
			motorHalted_cnt=0;
		}
	}
	//Clear period value globally
	FGPeriod = 0; 
}

/**
 * \brief Converts FG period measured with Timer to actual RPM
 * 				Must be called in corresponding Input Capture interrupt
 */
uint8_t BLDC_FG_PulseDetected(void){
		motorHalted_cnt=0;
		//Start blinking LED
		LED_Set(LED_GREEN, LED_BLINK);
	
		//Read current pulse value
		FGPeriod = TIM_GetCapture1(BLDC_FG_TIMER);
		//reset timer for new pulse detection
		TIM_SetCounter(BLDC_FG_TIMER, 0);
	
		//If FGPeriod averaging is enabled
#if BLDC_FG_AVERAGING_COUNT != 0
		uint8_t cnt1;
		uint32_t TotalLength = 0;
		//store current value
		FGPeriodHist[FGPeriodHistInd] = FGPeriod;

		//recalculate index
		if ((FGPeriodHistCnt) < (BLDC_FG_AVERAGING_COUNT)) FGPeriodHistCnt++; //increase used array size if it is not full at the moment
		FGPeriodHistInd++;
		if (FGPeriodHistInd >= FGPeriodHistCnt) FGPeriodHistInd = 0;

		//get new pulse length
		for (cnt1 = 0; cnt1 < FGPeriodHistCnt; cnt1++) {
				TotalLength += FGPeriodHist[cnt1];
		}
		FGPeriod = (uint16_t) (TotalLength / FGPeriodHistCnt);
#endif //BLDC_FG_AVERAGING_COUNT
		
		/* Perform BLDC motor RPM correction only when FG pulse is received.
			 Reduces overshooting at low RPM */
		RPM_Adjust_requested = true;
		// Adjust RPM at every FG pulse in BLDC startup mode (not every 100 ms as in normal mode)
		if (BLDC_Startup_Mode == true) {return BLDC_RPM_control();}
		return 1;
}

/**
 * @brief  Stabilisation of BLDC RPM 
 *				 must be called every 100 ms (or other period - depends on application)
 */
uint8_t BLDC_RPM_control(void){
	uint32_t RPM_Act = 0;
	//Print out BLDC RPM, PWM timer value, FGPeriod for debugging
	//printf("RPM: %d, %d, %d, %d\r\n", BLDC_RPM_target, BLDC_getRPM(),  TIM_GetCapture2(BLDC_PWM_TIMER), BLDC_getFGPeriod());
	
	
	//If adjusment is requested and motor is turned on
	if (RPM_Adjust_requested && (BLDC_getPower() == 1)){
		RPM_Adjust_requested = false;
		
		//Get actual RPM
		RPM_Act = BLDC_getRPM();
		
		//Do not adjust if RPM speed is not requested or motor is not running
		if ((BLDC_RPM_target == 0) || (RPM_Act == 0)) return 0; 
			
		//adjusment required
		if (BLDC_RPM_target != RPM_Act) {
			uint16_t	Step;
			int64_t 	Difference = 0;
			//Get current PWM parameter
			int64_t PWM_Pulse = TIM_GetCapture2(BLDC_PWM_TIMER);;
			
			//Calculate RPM difference
			Difference = BLDC_RPM_target - RPM_Act;
			
			//Select adjusment step, dependent on actual RPM difference
			if (abs(Difference) > 2000) Step = 100;
			else if (abs(Difference) > 100) Step = 10;
			else Step = 1;
			
			//Change PWM parameter if RPM can be adjusted more closely to target value. 
			//Actual RPM is lower than requested
			if (BLDC_RPM_target > (RPM_Act + (BLDC_RPM_ADJUSTEMT_STEP))) {
				uint32_t MaxPossiblePulseLength = (BLDC_PWM_TIMER_FREQ / BLDC_PWM_FREQ); //Get max allowed PWM pulse length
				PWM_Pulse += Step; //Calculate new pulse length		
				if (PWM_Pulse > MaxPossiblePulseLength) PWM_Pulse = MaxPossiblePulseLength; //Check for overflow
			}
			//Actual RPM is higher than requested
			else if ((BLDC_RPM_target  + (BLDC_RPM_ADJUSTEMT_STEP)) < RPM_Act){
				//BLDC startup mode
				if (BLDC_Startup_Mode == true){
					BLDC_Startup_Mode = false; //Required RPM achieved, switch to normal mode
					PWM_Pulse = RPM2PWM(BLDC_RPM_target); //calculate requested PWM pulse length based on target RPM
				}
				//BLDC normal mode
				else {
					uint32_t PWM_Pulse_Min = RPM2PWM(BLDC_RPM_MIN); //Get min allowed PWM pulse length
					PWM_Pulse -= Step; //Calculate new pulse length			
					if (PWM_Pulse < PWM_Pulse_Min) PWM_Pulse = PWM_Pulse_Min; //Motor will stop if pulse will be too low
				}
			}
			//Set new PWM parameter
			TIM_SetCompare2(BLDC_PWM_TIMER, (uint32_t) PWM_Pulse); /* Set the Capture Compare Register value Directly*/
		}
	}
	return 1;
}

/** *****************************************
*				Exported Functions - operation
****************************************** */

/**
 * \brief Turns on power for BLDC motor
 */
uint8_t BLDC_powerOn(void){
	//Reset value. It might be left unreset from previous operation if FGPeriod timer has not expired yet
	FGPeriod = 0; 
	GPIO_SetBits(BLDC_POWER_GPIO_PORT,BLDC_POWER_PIN);
	return 1;
}

/**
 * \brief Turns off the power for BLDC motor
 */
uint8_t BLDC_powerOff(void){
	BLDC_RPM_target = 0;
	BLDC_setPWM(0);
	GPIO_ResetBits(BLDC_POWER_GPIO_PORT,BLDC_POWER_PIN);
	return 1;
}

/**
 * \brief Get status of BLDC motor power
 */
uint8_t BLDC_getPower(void){
	return GPIO_ReadOutputDataBit(BLDC_POWER_GPIO_PORT, BLDC_POWER_PIN);
}

/**
 * \brief Sets the target RPM for BLDC motor
 * \param rpm		The target RPM value
 *
 */
uint8_t BLDC_setRPM(uint32_t rpm){
	uint32_t pwm = 0;
	bool BLDC_Startup_flag = false;
	
	//calculate new PWM pulse length for requested RPM
	pwm = RPM2PWM(rpm);

	/********* Startup mode *******
	*	BLDC_STARTUP_PWM will be held until required RPM will be achieved
	*	Go into startup mode if startup PWM is higher than new required pwm and motor is not running
	*	Remark:	Reading BLDC_getRPM() instead of BLDC_RPM_target is not recommended because 
	*					actual RPM values might be falsified during BLDC power switching
	*/
	if (((BLDC_STARTUP_PWM * 10) > pwm) && (BLDC_RPM_target == 0)) { 
		//Set startup PWM pulse length
		pwm = BLDC_STARTUP_PWM * 10; //Internal PWM values are 0 - 1000
		//Set BLDC startup flag to for later processing
		BLDC_Startup_flag = true;
	}
	
	//set new PWM
	if (BLDC_setPWM(pwm) != 0){
		//New PWM was set successfully
		//store requested rpm globaly for rpm adjusment routine
		BLDC_RPM_target = rpm;
		//Set BLDC mode
		BLDC_Startup_Mode = BLDC_Startup_flag;
	} else {
		//error occured
		return 0;
	}
	
	return 1;
}

/**
 * \brief Sets the PWM width for BLDC motor and starts or stops corresponding timer
 * \param rpm		The PWM width value
 *
 */
uint8_t BLDC_setPWM(uint32_t pwm){
	//motor is not turned on
	if (BLDC_getPower() == 0) {
		//Stop PWM
		TIM_OCInitStructure.TIM_Pulse = 0;
		TIM_OC2Init(BLDC_PWM_TIMER, &TIM_OCInitStructure);
		return 0;
	}
	
	TIM_OCInitStructure.TIM_Pulse = (uint16_t) pwm;
  TIM_OC2Init(BLDC_PWM_TIMER, &TIM_OCInitStructure);
	return 1;
}

/**
*  \brief Returns the measured motor RPM
*	 \retval Motor period in RPM
 */
uint32_t BLDC_getRPM(void){
	return FG2RPM(FGPeriod);
}
/**
 * \brief Gets the PWM duty cycle for BLDC motor
 * \return pwm		The PWM duty cycle value of PWM timer
 *
 */
uint8_t BLDC_getPWM(){
	uint8_t Duty;
	uint32_t Pulse, Period;
	
	Period = BLDC_PWM_TIMER_FREQ / BLDC_PWM_FREQ;
	Pulse = TIM_GetCapture2(BLDC_PWM_TIMER);
	Duty = (uint8_t)((Pulse * 100) / Period);
	return Duty;
}

/**
*  \brief Returns the measured FGPeriod value
*	 \retval FGPeriod value
 */
uint32_t BLDC_getFGPeriod(void){
	return FGPeriod;
}

/**
 * \brief Set direction of BLDC motor
 */
uint8_t BLDC_SetDirection(BLDC_DirectionTypeDef Dir){
	if (Dir == BLDC_CLOCKWISE) 
		GPIO_ResetBits(BLDC_DIR_GPIO_PORT, BLDC_DIR_PIN);
	else 
		GPIO_SetBits(BLDC_DIR_GPIO_PORT, BLDC_DIR_PIN);
	return 1;
}

/**
 * \brief Get direction of BLDC motor
 * \retval Direction in BLDC_DirectionTypeDef type
 */
BLDC_DirectionTypeDef BLDC_GetDirection(void){
	if (GPIO_ReadOutputDataBit(BLDC_DIR_GPIO_PORT, BLDC_DIR_PIN) == Bit_SET) 
		return BLDC_COUNTER_CLOCKWISE;
	else
		return BLDC_CLOCKWISE;
}

/** *****************************************
*				Internal Functions									
****************************************** */

/**
*		\brief Convert FG input value to RPM
*		\param Freq - PWM signal frequency
*		\param RPM - engine speed in RPM
*		\retval Pulse - PWM timer compare value
*/
static inline uint16_t FG2RPM (uint16_t FGPeriod){
	uint32_t Speed = 0;
	//one step of period is 10 us
	if (FGPeriod == 0) Speed = 0; //
	else Speed = ((60 * 100000 / FGPeriod) / 3); //(60 seconds * 100000 (convert period from 10 us to 1 s) / Period) / 3 pulses per round
	return (uint16_t) Speed;
}

/**
 * \brief Calculates the PWM needed to reach target RPM
 * \param rpm		The target RPM value
 *
 */
static inline uint32_t  RPM2PWM(uint32_t rpm){
	uint32_t pwm=rpm*10000/93848;
	if (pwm>1000) 
		pwm=1000;
	return pwm;
	
/** alternative calculation method with formula PWM = (RPM / RPM_DUTY_K) + RPM_DUTY_X0
 *	used when RPM_DUTY_X0 != 0. This condition can occur at higher PWM frequencies.
 *	At freqPWM = 50kHz: RPM_DUTY_K = 93; RPM_DUTY_X0 = 7.
*/
//	uint32_t Period, Duty, Pulse;
//	//RPM -> Duty Cycle conversion formula: PWM = (RPM / RPM_DUTY_K) + RPM_DUTY_X0
//	Duty = (((rpm * 100) / RPM_DUTY_K) + (RPM_DUTY_X0 * 100)); //Multiply by 100 to increase precision		
//	Period = BLDC_PWM_TIMER_FREQ / BLDC_PWM_FREQ; //Period of PWM generator timer
//	Pulse = ((Period * Duty) / 100) / 100; //Divide by 100 because Duty was multiplied
//	if (Pulse > 0xFFFF) Pulse = 0xFFFF; //Max possible value in register is 0xFFFF = 100% PWM
//	return (uint16_t) Pulse;
}


/**
  * @brief  Configure the GPIO.
  * @param  None
  * @retval None
  */
static void BLDC_GPIO_Config(void){
	
	/* GPIOC Periph clock enable */
  RCC_AHBPeriphClockCmd(BLDC_POWER_GPIO_CLK, ENABLE);
	
	/* Configure POWER PIN as output pushpull mode */
  GPIO_InitStructure.GPIO_Pin = BLDC_POWER_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(BLDC_POWER_GPIO_PORT, &GPIO_InitStructure);
	
	/* Configure DIR PIN as output pushpull mode */
  GPIO_InitStructure.GPIO_Pin = BLDC_DIR_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(BLDC_DIR_GPIO_PORT, &GPIO_InitStructure);
	
  /* GPIOA Configuration: PWM out - BLDC_PWM_TIMER CH2 (PB5) */
  GPIO_InitStructure.GPIO_Pin = BLDC_PWM_PIN ;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(BLDC_PWM_GPIO_PORT, &GPIO_InitStructure); 
    
  /* Connect TIM Channels to AF2 */
  GPIO_PinAFConfig(BLDC_PWM_GPIO_PORT, BLDC_PWM_SOURCE, BLDC_PWM_AF);
}

/**
  * @brief  Configure the FG TIMER and pin for RPM measurement.
  * @param  None
  * @retval None
  */
static void TIM_FG_Config(void)
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  TIM_ICInitTypeDef  TIM_ICInitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  
  /* TIM clock enable */
  RCC_APB2PeriphClockCmd(BLDC_FG_TIMER_CLK, ENABLE);

  /* GPIO clock enable */
  RCC_AHBPeriphClockCmd(BLDC_FG_GPIO_CLK, ENABLE);
  
  /* TIM channel pin configuration */
  GPIO_InitStructure.GPIO_Pin =  BLDC_FG_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //Enable pull-up to prevent pin floating while BLDC is disconnected
  GPIO_Init(BLDC_FG_GPIO_PORT, &GPIO_InitStructure);

  /* Connect TIM pins to AF2 */
  GPIO_PinAFConfig(BLDC_FG_GPIO_PORT, BLDC_FG_SOURCE, BLDC_FG_AF);
    
  /* ---------------------------------------------------------------------------
    BLDC_FG_TIMER Configuration
    Note: 
     BLDC_FG_CLOCK_BASE (SystemCoreClock) variable holds HCLK frequency and is defined in system_stm32f0xx.c file.
     Each time the core clock (HCLK) changes, user had to call SystemCoreClockUpdate()
     function to update SystemCoreClock variable value. Otherwise, any configuration
     based on this variable will be incorrect. 
  --------------------------------------------------------------------------- */
  
  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 0xFFFF; //Set max timer period
  TIM_TimeBaseStructure.TIM_Prescaler = (uint16_t)((BLDC_FG_CLOCK_BASE / BLDC_FG_TIMER_FREQ) - 1);
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(BLDC_FG_TIMER, &TIM_TimeBaseStructure);

	
  /* TIM1 configuration: Input Capture mode ---------------------
  ------------------------------------------------------------ */

  TIM_ICInitStructure.TIM_Channel = BLDC_FG_TIM_CH;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
  TIM_ICInitStructure.TIM_ICFilter = 0x0;

  TIM_ICInit(BLDC_FG_TIMER, &TIM_ICInitStructure);
  
  /* TIM enable counter */
  TIM_Cmd(BLDC_FG_TIMER, ENABLE);

  /* Enable the TIMx global Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = BLDC_FG_TIM_IRQ;
  NVIC_InitStructure.NVIC_IRQChannelPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
  /* Enable the CCx and Timer upadate Interrupt Requests*/
  TIM_ITConfig(BLDC_FG_TIMER, (BLDC_FG_CC), ENABLE);  
  TIM_ITConfig(BLDC_FG_TIMER, (BLDC_FG_Update), ENABLE);  
}

/**
  * @brief  Configure the motor PWM TIMER and pins.
  * @param  None
  * @retval None
  */
static void TIM_PWM_Config(void)
{
	uint16_t CCR_Val = 10;
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

  /* BLDC_PWM_TIMER clock enable */
  RCC_APB1PeriphClockCmd(BLDC_PWM_TIMER_CLK, ENABLE);

  /* GPIO clock enable */
  RCC_AHBPeriphClockCmd(BLDC_PWM_GPIO_CLK, ENABLE);
 
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
  TIM_TimeBaseStructure.TIM_Period = (uint16_t)(BLDC_PWM_TIMER_FREQ / BLDC_PWM_FREQ);
  TIM_TimeBaseStructure.TIM_Prescaler = (uint16_t)((BLDC_PWM_CLOCK_BASE / BLDC_PWM_TIMER_FREQ) - 1);
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(BLDC_PWM_TIMER, &TIM_TimeBaseStructure);

  /* Output Compare Active Mode configuration: Channel2 */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

  TIM_OCInitStructure.TIM_Pulse = CCR_Val;
  TIM_OC2Init(BLDC_PWM_TIMER, &TIM_OCInitStructure);
	
  TIM_ARRPreloadConfig(BLDC_PWM_TIMER, DISABLE); 
  TIM_OC2PreloadConfig(BLDC_PWM_TIMER, TIM_OCPreload_Disable);
 
  /* BLDC_PWM_TIMER enable counter */
  TIM_Cmd(BLDC_PWM_TIMER, ENABLE);
  
  TIM_GenerateEvent(BLDC_PWM_TIMER, TIM_EventSource_Update);
}
