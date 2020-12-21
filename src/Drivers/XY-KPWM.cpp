#include <Arduino.h>
#include "XY-KPWM.h"
#include "../board.h"

// keep track of values needed to read PWM signal
volatile unsigned long timeoldPWM = 0; // used to calculate the PWM duty cycle from the XY-KPWM
volatile unsigned long pwm_count = 0; // used to check if we are receiving a pwm signal from XY-KPWM
volatile unsigned long pwm_count_old = 0;
volatile long pwm_value = 0; // The PWM value in microseconds
unsigned long sumPWMSpeed = 0; // Sum of PWM value for averaging
unsigned long pwmSpeedOld = 0;
boolean changePWMSpeed = false;
unsigned long pwmSpeed = 0; // rpm speed set by pwm signal

void risingPWM() {
  attachInterrupt(XY_XPWM_PIN, fallingPWM, FALLING);
  
  timeoldPWM = micros();
}

void fallingPWM() {
  attachInterrupt(XY_XPWM_PIN, risingPWM, RISING);
  
  pwm_value = micros() - timeoldPWM;
  sumPWMSpeed += pwm_value;
  pwm_count++;

  // average the speed
  if(pwm_count % 100 == 0) { 
    long speed10 = sumPWMSpeed/100; // average 100 readings
    speed10 = (speed10 + 5)/10;
    speed10 = 10*speed10;
    pwmSpeed = speed10;

    // check to if the speed was moved
    if(pwmSpeed != pwmSpeedOld) {
      pwmSpeedOld = pwmSpeed;
      changePWMSpeed = true;
    } else {
      changePWMSpeed = false;
    }

    // reset the speed sum
    sumPWMSpeed = 0;
  }
}



void XY_KPWM_Init(void) {
  pinMode(XY_XPWM_PIN, INPUT_PULLUP);
    // used to measure pwm signal
  //TCCR1B = TCCR1B & 0b11111000 | 0x01; // set PWM frequency to 31khz
  attachInterrupt(XY_XPWM_PIN, risingPWM, RISING);  
}  
