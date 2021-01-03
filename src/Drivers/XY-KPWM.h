#pragma once

#define XY_KPWM_TIMER_FREQ 8000000
#define XY_KPWM_PWM_freq 1000

extern volatile unsigned long XY_Speed; // rpm speed set by pwm signal
extern volatile bool changeXYSpeed;
extern bool XYEnabled;

void XY_KPWM_Init();
void XY_XPWM_Process();
