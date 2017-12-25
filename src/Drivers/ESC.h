#ifndef _ESC_H_
#define _ESC_H_

#include <stdbool.h>
#include "stm32f0xx.h"
#include "board.h"
//-------------- ESC I2C definitions -----------

#define ESC_ADDR        0x29
#define ESC_POLE_COUNT  7

//-------------- ESC motor control -------------
// ESC is stopped
#define ESC_THROTTLE_MIN                        0
// Max forward or reverse (depending on sign)
#define ESC_THROTTLE_MAX                        32767
//min allowed RPM value
#define ESC_RPM_MIN                        250
//max allowed RPM value
#define ESC_RPM_MAX                        15000
//Minimum possible motor adjustment step in RPM. Depends on max motor speed (at 100% duty) and PWM timer resolution.
#define ESC_RPM_ADJUSTMENT_STEP    9
//PWM
#define ESC_PWM_MIN             10
#define ESC_PWM_MAX             1000
//Default PWM to be set at motor startup (0 - 100); 0 - do not use ESC_STARTUP_PWM
#define    ESC_STARTUP_PWM_DEF                10
// MOTOR PROFILE
// 820 Set the motor profile default slope * 100 for estimating PWM based on RPM
#define ESC_SLOPE_DEF                            850
// Set the motor profile default intercept for estimating PWM based on RPM
#define ESC_INTERCEPT_DEF                    1800


//Exported definitions
typedef enum {
    ESC_COUNTER_CLOCKWISE = 0,
    ESC_CLOCKWISE = 1
} ESC_DirectionTypeDef;

//Exported functions
//Must be called from initialization section
int ESC_init(void);

//Must be called from initialization section
int ESC_deInit(void);

//Must be called every 100 ms
uint8_t ESC_RPM_control(void);

uint8_t ESC_setRPM(uint32_t rpm);

uint32_t ESC_getRPM(void);

uint8_t ESC_setPWM(uint32_t pwm);

uint32_t ESC_getPWM(void);

uint8_t ESC_setStartupPWM(uint32_t pwm);

uint32_t ESC_getStartupPWM(void);

uint8_t ESC_setSlope(uint32_t slope);

uint32_t ESC_getSlope(void);

uint8_t ESC_setIntercept(uint32_t intercept);

uint32_t ESC_getIntercept(void);

int ESC_powerOn(void);

int ESC_powerOff(void);

bool ESC_isPoweredOn(void);

uint8_t ESC_SetDirection(ESC_DirectionTypeDef Dir);

ESC_DirectionTypeDef ESC_GetDirection(void);

#endif
