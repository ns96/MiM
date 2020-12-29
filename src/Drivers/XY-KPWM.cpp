#include <Arduino.h>
#include "XY-KPWM.h"
#include "../board.h"

static volatile unsigned long pwm_count = 0; // used to check if we are receiving a pwm signal from XY-KPWM
static volatile unsigned long sumPWMDuty = 0; // Sum of PWM value for averaging
static volatile unsigned long XY_SpeedOld = 0;
volatile bool changeXYSpeed = false;
volatile unsigned long XY_Speed = 0; // rpm speed set by pwm signal
static volatile bool XY_XPWM_int = false;
bool XYEnabled = false;
static volatile bool XY_init_mode = true;

/**
  * @brief  Interrupt for XY_XPW when read signal duty
  * @param  None
  * @retval None
  */
ISR(TCB2_INT_vect) {
  unsigned long duty = XY_KPWM_TIMER.CCMP; // reading CCMP clears interrupt flag
  XY_XPWM_int = true;

  sumPWMDuty += duty;
  pwm_count++;

  // average the speed
  if (pwm_count == 5) {
    unsigned long speed10 = sumPWMDuty / 5; // average n readings
	//convert pwm duty to RPM
	//convert timer cnt to microsecond period
	speed10 = (125 * speed10) / 1000;
	speed10 = (speed10 + 5)/10;
	speed10 = 100 * speed10;
    XY_Speed = speed10;
    pwm_count = 0;
    // reset the speed sum
    sumPWMDuty = 0;
	if (XY_Speed > 10000) {
		XY_Speed = 10000;
	}
	
	if (XY_SpeedOld != XY_Speed) {
		XY_SpeedOld = XY_Speed;
		changeXYSpeed = true;
		XY_init_mode = false;
	}
  }
}

/**
  * @brief  Check that interrupt was read for time interval
  *				 must be called every 1 s
  * @param  None
  * @retval None
  */
void XY_XPWM_Process(void) {
	if (XY_XPWM_int) {
      XY_XPWM_int = false;
	  return;
	}
    
	XY_Speed = 0;
	//PWM signal not detected for XY_XPWM 
	if (digitalRead(XY_XPWM_PIN) && !XY_init_mode) {
		XY_Speed = 10000;
	}
	if (XY_SpeedOld != XY_Speed) {
		XY_SpeedOld = XY_Speed;
		changeXYSpeed = true;
	}
}

/**
  * @brief  Configure the GPIO and Timer.
  * @param  None
  * @retval None
  */
void XY_KPWM_Init(void) {
  //configure pin	
  pinMode(XY_XPWM_PIN, INPUT);
  EVSYS.CHANNEL1 = XY_XPWM_PIN_EVSYS_PORT; // Route XY_KPWM pin to EVSYS.CHANNEL1; 
  EVSYS.USERTCB2 = EVSYS_CHANNEL_CHANNEL1_gc;
  
  //configure timer for reading signal duty 
  XY_KPWM_TIMER.CTRLA = 0; // Turn off channel for configuring
  XY_KPWM_TIMER.CTRLB = 0 << TCB_ASYNC_bp      /* Asynchronous Enable: disabled */
               | 0 << TCB_CCMPEN_bp   /* Pin Output Enable: disabled */
               | 0 << TCB_CCMPINIT_bp /* Pin Initial State: disabled */
               | TCB_CNTMODE_PW_gc;   /* Input Capture Pulse-Width measurement */

  // TCB0.DBGCTRL = 0 << TCB_DBGRUN_bp; /* Debug Run: disabled */

  XY_KPWM_TIMER.EVCTRL = 1 << TCB_CAPTEI_bp    /* Event Input Enable: enabled */
                | 0 << TCB_EDGE_bp    /* Event Edge: disabled */
                | 0 << TCB_FILTER_bp; /* Input Capture Noise Cancellation Filter: enabled */

  XY_KPWM_TIMER.INTCTRL = 1 << TCB_CAPT_bp /* Capture or Timeout: enabled */;

  XY_KPWM_TIMER.CTRLA = TCB_CLKSEL_CLKDIV2_gc  /* CLK_PER/2 (From Prescaler) */
               | 1 << TCB_ENABLE_bp   /* Enable: enabled */
               | 0 << TCB_RUNSTDBY_bp /* Run Standby: disabled */
               | 0 << TCB_SYNCUPD_bp; /* Synchronize Update: disabled */  
}  
