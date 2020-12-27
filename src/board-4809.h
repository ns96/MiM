/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BOARD_H
#define __BOARD_H

#define Bit_SET 1

/*****************************************************************
 * @brief Definition for BLDC motor pins and timers
 *****************************************************************/ 

#define BLDC_DIR_PIN		 	8 //D8
#define BLDC_FG_PIN		 		2 //D2
#define BLDC_PWM_PIN		 	9 //D9

#define BLDC_FG_PIN_EVSYS_PORT EVSYS_GENERATOR_PORT0_PIN0_gc; // Route to FG pin PA0
#define FG_TIMER TCB0
#define BLDC_PWM_PORTMUX		PORTMUX_TCA0_PORTB_gc	//route PWM ouput to PB0 (D9)

/*****************************************************************
 * @brief Definition for STEP motor pins and timers
 *****************************************************************/ 
#define STEP_ENABLE_PIN			10 //D10	
#define STEP_DIR_PIN			12 //D12	
#define STEP_PWM_PIN			11 //D11

#define STEP_PWM_PORTMUX		PORTMUX_TCA0_PORTE_gc	//route PWM ouput to PE0 (D11)
/*****************************************************************
 * @brief Definition for SWITCH and GP input pins
 *****************************************************************/ 
#define S01_PIN       		    14 // A0
#define S02_PIN       		    15 // A1

/*****************************************************************
 * @brief Definition for LED pins
 *****************************************************************/ 
#define LED_RED_PIN   		    4 // D4
#define LED_BLUE_PIN  		    6 // D6
#define LED_GREEN_PIN  			5 // D5

/*****************************************************************
 * @brief Definition for XY-PWM pin
 *****************************************************************/ 

#define XY_XPWM_PIN   			3 //D3 reads XY-KPWM pwm signal
#define XY_XPWM_PIN_EVSYS_PORT	EVSYS_GENERATOR_PORT1_PIN5_gc // Route to XY-KPWM pin PF5 (D3) 
#define XY_KPWM_TIMER			TCB1
#endif
