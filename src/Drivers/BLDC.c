#include <Arduino.h>
#include <stdint.h>
#include "BLDC.h"
#include "LED.h"
#include <stdbool.h>
#include <stdlib.h>

//-------------- private variables -------------
	static volatile bool RPM_Adjust_requested = false, BLDC_Startup_Mode = false, Skip_FG_Pulse = false, BLDC_power = 0;
	static volatile uint32_t BLDC_RPM_target = 0;
	volatile uint16_t FGPeriod = 0; //Period of FG signal
	volatile uint32_t FGPeriod2 = 0; //Period of FG signal
	static volatile uint32_t motorHalted_cnt=0;
	static volatile uint32_t loopCount = 0; //Used to remain in startupMode for 1 second to smooth out starting at low rpms
	static volatile uint32_t BLDC_Startup_PWM = BLDC_STARTUP_PWM_DEF;		//Default PWM to be set at motor startup
	static volatile uint32_t BLDC_Slope = BLDC_SLOPE_DEF;			//Motor profile parameter
	static volatile uint32_t BLDC_Intercept = BLDC_INTERCEPT_DEF; //Motor profile parameter
	//If FGPeriod averaging is going to be used
	#if BLDC_FG_AVERAGING_COUNT != 0
	static uint8_t  FGPeriodHistInd = 0, FGPeriodHistCnt = 0; //History of pulse length required for averaging: index in array and registered pulses count
	static uint16_t FGPeriodHist[BLDC_FG_AVERAGING_COUNT]; //History of pulse length required for averaging
	#endif
	volatile uint32_t FG_Timer_Timeouts = 0;

//---------------  Internal functions prototypes -------------------
	static inline uint16_t FG2RPM (uint16_t FGPeriod);
	static inline uint32_t  RPM2PWM(uint32_t rpm);
	static uint8_t BLDC_setTimerPWM(uint32_t pwm);
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
	if ((TCA0.SINGLE.CTRLB & (1 << TCA_SINGLE_CMP0EN_bp)) > 0){
		//Increment motor halted timer. 
		//No signs of motor spinning for ~10seconds?
		if (++motorHalted_cnt>10){
			//Set motor control values to defaults
			BLDC_setTimerPWM(0);
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
 * \brief Interrupt for reading FG frequency
 * 
 */
ISR(TCB0_INT_vect) {
	FGPeriod2 = FG_TIMER.CCMP; // reading CCMP clears interrupt flag
	//start FG timeout timer
	FG_TIMER_OVERFLOWS.CNT = 0;

	if ((FG_TIMER_OVERFLOWS.CTRLA & (1 << TCB_ENABLE_bp)) == 0){
      FG_TIMER_OVERFLOWS.CTRLA |= (1 << TCB_ENABLE_bp);
		FG_Timer_Timeouts = 0;
		return;
	}

	FGPeriod2 = FGPeriod2 + FG_Timer_Timeouts * 65535;
	//convert period 8 MHZ timer to period of 100000 HZ timer
	FGPeriod2 /= 80;
	FG_Timer_Timeouts = 0;
	BLDC_FG_PulseDetected(FGPeriod2);
	
}

/**
 * \brief Interrupt for calculating timeouts of FG timer
 * 
 */
ISR(TCB1_INT_vect)
{
	FG_Timer_Timeouts++;
	// 1 sec interval passed without FG timeouts
	if (FG_Timer_Timeouts >= 122) {
		FG_Timer_Timeouts = 0;
		BLDC_FG_PulseMissing();
	}
    FG_TIMER_OVERFLOWS.INTFLAGS = TCB_CAPT_bm;
}

/**
 * \brief Converts FG period measured with Timer to actual RPM

 * 				Must be called in corresponding Input Capture interrupt
 */

uint8_t BLDC_FG_PulseDetected(uint16_t CapturedValue){

		motorHalted_cnt=0;
	
		//Ignore first FG pulse. Fake pulse is generated when motor's power is switched
		if (Skip_FG_Pulse) {
			Skip_FG_Pulse = false;
			return 0;
		} else {				
			//Start blinking LED
			LED_Set(LED_GREEN, LED_BLINK);
				
			//set new FGPeriod value
			FGPeriod = CapturedValue;
			
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
		}
		return 1;
}

/**
 * @brief  Stabilisation of BLDC RPM 
 *				 must be called every 100 ms (or other period - depends on application)
 */
uint8_t BLDC_RPM_control(void){
	uint32_t RPM_Act = 0;
	//Print out BLDC RPM, PWM timer value, FGPeriod and other for debugging
	//printf("RPM: %d, %d, %d, %d, %d\r\n", BLDC_RPM_target, BLDC_getRPM(),  TIM_GetCapture2(BLDC_PWM_TIMER), BLDC_getFGPeriod(), motorHalted_cnt);
	//printf("BLDC: %d, %d, %d\r\n", BLDC_Startup_PWM, BLDC_Slope,  BLDC_Intercept);
	
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
			int64_t PWM_Pulse = 1000 - TCA0.SINGLE.CMP0BUF;
						
			//Calculate RPM difference
			Difference = BLDC_RPM_target - RPM_Act;
			
			//Select adjusment step, dependent on actual RPM difference
			if (abs(Difference) > 2000) Step = 20;
			else if (abs(Difference) > 100) Step = 5;
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
					// make sure this code runs for 10 seconds to smooth out motor start
					loopCount++;
					if(loopCount >= 5) {
						loopCount = 0;
						BLDC_Startup_Mode = false; //Required RPM achieved, switch to normal mode
					}
					
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
			TCA0.SINGLE.CMP0BUF = (uint16_t) (1000 - PWM_Pulse);
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
	FGPeriod = 0; //Reset value. It might be left unreset from previous operation if FGPeriod timer has not expired yet
	Skip_FG_Pulse = true; //Ignore first FG pulse. Fake pulse is generated when motor's power is switched
//	GPIO_SetBits(BLDC_POWER_GPIO_PORT,BLDC_POWER_PIN); //Set/Clear POWER PIN
	BLDC_GPIO_Config();
	TIM_PWM_Config();
	TIM_FG_Config();
	BLDC_power = 1;
	return 1;
}

/**
 * \brief Turns off the power for BLDC motor
 */
uint8_t BLDC_powerOff(void){
	BLDC_RPM_target = 0;  //Request RPM = 0
	Skip_FG_Pulse = true; //Ignore first FG pulse. Fake pulse is generated when motor's power is switched
	BLDC_setTimerPWM(0);				//Stop PWM generation
//	GPIO_ResetBits(BLDC_POWER_GPIO_PORT,BLDC_POWER_PIN); //Set/Clear POWER PIN
	BLDC_power = 0;
	// Disable FG overflows timer
	FG_TIMER_OVERFLOWS.CTRLA &= ~(1 << TCB_ENABLE_bp);
	return 1;
}

/**
 * \brief Get status of BLDC motor power
 */
uint8_t BLDC_getPower(void){
//	return GPIO_ReadOutputDataBit(BLDC_POWER_GPIO_PORT, BLDC_POWER_PIN);
	return BLDC_power;
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
	if (((BLDC_Startup_PWM * 10) > pwm) && (BLDC_RPM_target == 0) && (pwm != 0)) { 
		//Set startup PWM pulse length
		pwm = BLDC_Startup_PWM * 10; //Internal PWM values are 0 - 1000
		//Set BLDC startup flag to for later processing
		BLDC_Startup_flag = true;
	}
	
	//set new PWM
	if (BLDC_setTimerPWM(pwm) != 0){
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
 * \brief Sets the PWM width for BLDC motor and starts it
 * \param rpm		The PWM width value 0 - 1000
 *
 */
uint8_t BLDC_setPWM(uint32_t pwm){
	BLDC_RPM_target = 0;  //Exit RPM control mode to keep requested PWM
	return BLDC_setTimerPWM(pwm);
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
uint32_t BLDC_getPWM(){
	uint32_t Duty;
	uint32_t Pulse, Period;
	
	Period = BLDC_PWM_TIMER_FREQ / BLDC_PWM_FREQ;
	Pulse = TCA0.SINGLE.CMP0;
	Duty = (uint32_t)((Pulse * 1000) / Period);
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
		digitalWrite(BLDC_DIR_PIN,LOW);
	else 
		digitalWrite(BLDC_DIR_PIN,HIGH);
	return 1;
}

/**
 * \brief Get direction of BLDC motor
 * \retval Direction in BLDC_DirectionTypeDef type
 */
BLDC_DirectionTypeDef BLDC_GetDirection(void){
	if (digitalRead(BLDC_DIR_PIN) == Bit_SET) 
		return BLDC_COUNTER_CLOCKWISE;
	else
		return BLDC_CLOCKWISE;
}

/**
 * \brief Set StartupPWM of BLDC motor.
 * \param pwm		The PWM width value (0 - 100). 0 - do not use StartupPWM function
 */
uint8_t BLDC_setStartupPWM(uint32_t pwm){
	if (pwm > 100) 
		return 0;
	BLDC_Startup_PWM = pwm;
	return 1;
}

/**
*  \brief Returns the StartupPWM value
*	 \retval StartupPWM value
 */
uint32_t BLDC_getStartupPWM(void){
	return BLDC_Startup_PWM;
}


/**
 * \brief Set StartupPWM of BLDC motor.
 * \param pwm		The PWM width value (0 - 100). 0 - do not use StartupPWM function
 */
uint8_t BLDC_setSlope(uint32_t slope){
	BLDC_Slope = slope;
	return 1;
}

/**
*  \brief Returns the StartupPWM value
*	 \retval StartupPWM value
 */
uint32_t BLDC_getSlope(void){
	return BLDC_Slope;
}

/**
 * \brief Set StartupPWM of BLDC motor.
 * \param pwm		The PWM width value (0 - 100). 0 - do not use StartupPWM function
 */
uint8_t BLDC_setIntercept(uint32_t intercept){
	BLDC_Intercept = intercept;
	return 1;
}

/**
*  \brief Returns the StartupPWM value
*	 \retval StartupPWM value
 */
uint32_t BLDC_getIntercept(void){
	return BLDC_Intercept;
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
	//uint32_t pwm=rpm*10000/93848;
	uint32_t pwm=((rpm + BLDC_Intercept) * 100) / BLDC_Slope; //Multiply value by 100 because BLDC_Slope parameter is also multiplied
	
	// check to make sure we not setting the rpm to a point where the motor
	// would fail to spin and just set pwm to zero to prevent motor auto protection
	if(rpm < (BLDC_RPM_MIN))
		pwm=0;
	
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
 * \brief Sets the PWM width for BLDC motor timer and starts it
 * \param rpm		The PWM width value 0 - 1000
 *
 */
static uint8_t BLDC_setTimerPWM(uint32_t pwm){
	//motor is not turned on
	if (BLDC_getPower() == 0) {
		//Stop PWM
		//disable output
		TCA0.SINGLE.CTRLB &= ~(1 << TCA_SINGLE_CMP0EN_bp);
		return 0;
	}
	
	TCA0.SINGLE.CMP0BUF = (uint16_t) (1000 - pwm);
	//enable output if disabled
	if ((TCA0.SINGLE.CTRLB & (1 << TCA_SINGLE_CMP0EN_bp)) == 0){
		TCA0.SINGLE.CTRLB |= (1 << TCA_SINGLE_CMP0EN_bp);
	}

	return 1;
}

/**
  * @brief  Configure the GPIO.
  * @param  None
  * @retval None
  */
static void BLDC_GPIO_Config(void){
	pinMode(BLDC_FG_PIN, INPUT_PULLUP);
	pinMode(BLDC_DIR_PIN, OUTPUT);	
	pinMode(BLDC_PWM_PIN, OUTPUT);
	digitalWrite(BLDC_PWM_PIN, LOW);
	PORTMUX.TCAROUTEA = BLDC_PWM_PORTMUX;
	
	//FG PIN
	EVSYS.CHANNEL0 = BLDC_FG_PIN_EVSYS_PORT;
	EVSYS.USERTCB0 = EVSYS_CHANNEL_CHANNEL0_gc; // to TCB0
}

/**
  * @brief  Configure the FG TIMER and pin for RPM measurement.
  * @param  None
  * @retval None
  */
static void TIM_FG_Config(void)
{
  
  FG_TIMER.CTRLA = 0; // Turn off channel for configuring
  FG_TIMER.CTRLB = 0 << TCB_ASYNC_bp      /* Asynchronous Enable: disabled */
               | 0 << TCB_CCMPEN_bp   /* Pin Output Enable: disabled */
               | 0 << TCB_CCMPINIT_bp /* Pin Initial State: disabled */
               | TCB_CNTMODE_FRQ_gc;  /* Input Capture Frequency measurement */

  // TCB0.DBGCTRL = 0 << TCB_DBGRUN_bp; /* Debug Run: disabled */

  FG_TIMER.EVCTRL = 1 << TCB_CAPTEI_bp    /* Event Input Enable: enabled */
                | 0 << TCB_EDGE_bp    /* Event Edge: disabled */
                | 1 << TCB_FILTER_bp; /* Input Capture Noise Cancellation Filter: enabled */

  FG_TIMER.INTCTRL = 1 << TCB_CAPT_bp /* Capture or Timeout: enabled */;

  FG_TIMER.CTRLA = TCB_CLKSEL_CLKDIV2_gc  /* CLK_PER/2 (From Prescaler) */
               | 1 << TCB_ENABLE_bp   /* Enable: enabled */
               | 0 << TCB_RUNSTDBY_bp /* Run Standby: disabled */
               | 0 << TCB_SYNCUPD_bp; /* Synchronize Update: disabled */
			   
	//Set up FG overflows Timer - counts overflows of FG_TIMER
	FG_TIMER_OVERFLOWS.CCMP = 0xffff; /* Compare or Capture: 0xffff */
	FG_TIMER_OVERFLOWS.CNT = 0x0; /* Count: 0x0 */

	 FG_TIMER_OVERFLOWS.CTRLB = 0 << TCB_ASYNC_bp /* Asynchronous Enable: disabled */
			 | 0 << TCB_CCMPEN_bp /* Pin Output Enable: disabled */
			 | 0 << TCB_CCMPINIT_bp /* Pin Initial State: disabled */
			 | TCB_CNTMODE_INT_gc; /* Periodic Interrupt */

	// TCB0.DBGCTRL = 0 << TCB_DBGRUN_bp; /* Debug Run: disabled */

	// TCB0.EVCTRL = 0 << TCB_CAPTEI_bp /* Event Input Enable: disabled */
	//		 | 0 << TCB_EDGE_bp /* Event Edge: disabled */
	//		 | 0 << TCB_FILTER_bp; /* Input Capture Noise Cancellation Filter: disabled */

	FG_TIMER_OVERFLOWS.INTCTRL = 1 << TCB_CAPT_bp /* Capture or Timeout: enabled */;

  FG_TIMER_OVERFLOWS.CTRLA = TCB_CLKSEL_CLKDIV2_gc  /* CLK_PER/2 (From Prescaler) */
	             | 0 << TCB_ENABLE_bp   /* Enable: disabled */
	             | 0 << TCB_RUNSTDBY_bp /* Run Standby: disabled */
	             | 0 << TCB_SYNCUPD_bp; /* Synchronize Update: disabled */
				 
}

/**
  * @brief  Configure the motor PWM TIMER and pins.
  * @param  None
  * @retval None
  */
static void TIM_PWM_Config(void)
{
	uint16_t CCR_Val = 10;
	
	TCA0.SINGLE.CTRLB = 0 << TCA_SINGLE_ALUPD_bp         /* Auto Lock Update: disabled */
	                    | 0 << TCA_SINGLE_CMP0EN_bp      /* Compare 0 Enable: disabled */
	                    | 0 << TCA_SINGLE_CMP1EN_bp      /* Compare 1 Enable: enabled */
	                    | 0 << TCA_SINGLE_CMP2EN_bp      /* Compare 2 Enable: disabled */
	                    | TCA_SINGLE_WGMODE_SINGLESLOPE_gc; /*  */
	
	TCA0.SINGLE.CMP0BUF = (1000 - CCR_Val); /* Compare Register 0: 0x10 */
	TCA0.SINGLE.PER = (uint16_t)(BLDC_PWM_TIMER_FREQ / BLDC_PWM_FREQ); /* Period: 0x3e8 */

	TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV2_gc /* System Clock / 2 */
	                    | 1 << TCA_SINGLE_ENABLE_bp /* Module Enable: enabled */;
  
}
