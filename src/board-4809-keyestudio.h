/**
 * Pinout for: Keyestudio CNC Shield
 */
#ifndef __BOARD_H
#define __BOARD_H

/*****************************************************************
 * @brief Definition for BLDC motor pins and timers
 *****************************************************************/

#define BLDC_DIR_PIN            8
#define BLDC_FG_PIN             2
#define BLDC_PWM_PIN            5 //(PB2)

#define BLDC_FG_PIN_EVSYS_PORT  EVSYS_GENERATOR_PORT0_PIN0_gc; // Route to FG pin PA0
#define FG_TIMER                TCB0
#define BLDC_PWM_PORTMUX        PORTMUX_TCA0_PORTB_gc    //route PWM output to PB0-PB2
#define FG_TIMER_OVERFLOWS      TCB1

#define BLDC_CMP                TCA0.SINGLE.CMP2
#define BLDC_CMPBUF             TCA0.SINGLE.CMP2BUF
#define BLDC_CMPEN_BP           TCA_SINGLE_CMP2EN_bp

/*****************************************************************
 * @brief Definition for STEP motor pins and timers
 *****************************************************************/
#define STEP_ENABLE_PIN         8
#define STEP_DIR_PIN            4
#define STEP_PWM_PIN            7 // (PA1)

#define STEP_PWM_PORTMUX        PORTMUX_TCA0_PORTA_gc    //route PWM output to PA0-PA2

#define STEP_ISR                TCA0_CMP1_vect
#define STEP_INTFLAG            TCA_SINGLE_CMP1_bm
#define STEP_CMPBUF             TCA0.SINGLE.CMP1BUF
#define STEP_CMP_BP             TCA_SINGLE_CMP1EN_bp
#define STEP_INTCTRL_BP         TCA_SINGLE_CMP1_bp

/*****************************************************************
 * @brief Definition for SWITCH and GP input pins
 *****************************************************************/
#define S01_PIN                 10
#define S02_PIN                 11

/*****************************************************************
 * @brief Definition for LED pins
 *****************************************************************/
#define LED_RED_PIN             A0
#define LED_GREEN_PIN           A1
#define LED_BLUE_PIN            A2

/*****************************************************************
 * @brief Definition for XY-PWM pin
 *****************************************************************/

#define XY_XPWM_PIN                      A6 //A6(PD4) reads XY-KPWM pwm signal
#define XY_XPWM_PIN_EVSYS_CHANNEL        CHANNEL2
#define XY_XPWM_PIN_EVSYS_GENERATOR      EVSYS_GENERATOR_PORT1_PIN4_gc // Route to XY-KPWM pin PD4
#define XY_XPWM_PIN_EVSYS_USER_CHANNEL   EVSYS_CHANNEL_CHANNEL2_gc // Route to XY-KPWM pin PD4
#define XY_KPWM_TIMER                    TCB2

#endif
