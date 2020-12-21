#pragma once


extern boolean changePWMSpeed;
extern unsigned long pwmSpeed; // rpm speed set by pwm signal

void XY_KPWM_Init(void);
void fallingPWM(void);
