#include "ESC.h"
#include "main.h"
#include "I2C_Software_Master.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stm32f0xx_tim.h>

#define ESC_REG_THROTTLE    0x00
#define ESC_REG_RPM         0x02

//-------------- private variables -------------
// Data read from ESC controller
static uint8_t buffer[9];
static volatile int16_t _rpm;
static volatile uint8_t _identifier;

static volatile bool RPM_Adjust_requested = false, ESC_Startup_Mode = false, ESC_powered_on = false;
static ESC_DirectionTypeDef direction = ESC_CLOCKWISE;
// current internal PWM value (0 - 1000)
static volatile uint32_t current_PWM;
static volatile int ESC_RPM_target = 0;
static volatile uint32_t loopCount = 0; //Used to remain in startupMode for 1 second to smooth out starting at low rpms
static volatile uint32_t ESC_Startup_PWM = ESC_STARTUP_PWM_DEF;        //Default PWM to be set at motor startup
static volatile uint32_t ESC_Slope = ESC_SLOPE_DEF;            //Motor profile parameter
static volatile uint32_t ESC_Intercept = ESC_INTERCEPT_DEF; //Motor profile parameter

//---------------  Internal functions prototypes -------------------

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/**
 * \brief Calculates the PWM needed to reach target RPM
 * \param rpm		The target RPM value
 * \returns pwm 0 - 1000
 */
static inline uint32_t RPM2PWM(uint32_t rpm) {
    // prevent unsigned underflow
    if (rpm < ESC_Intercept) {
        return 0;
    }

    //uint32_t pwm=rpm*10000/93848;
    //Multiply value by 100 because ESC_Slope parameter is also multiplied
    uint32_t pwm = ((rpm - ESC_Intercept) * 100) / ESC_Slope;

    // check to make sure we not setting the rpm to a point where the motor
    // would fail to spin and just set pwm to zero to prevent motor auto protection
    if (rpm < (ESC_RPM_MIN))
        pwm = 0;

    if (pwm > 1000)
        pwm = 1000;

    return pwm;
}


/**
 * Internal functions
 * see http://docs.bluerobotics.com/bluesc/#i2c-protocol
 * and https://github.com/bluerobotics/Arduino_I2C_ESC
 */

/**
 * Reads data over I2C from ESC controller
 * @returns 0 - error, 1 - OK
*/
static int readBuffer(void) {
    return I2C_SoftWare_Master_Read(ESC_ADDR, ESC_REG_RPM, sizeof(buffer), buffer) ? 1 : 0;
}

/**
 * Send motor speed command to ESC
 * @param throttle -32767 (max reverse) to 32767 (max forward), 0 - stop
 * @returns 0 - error, 1 - OK
 */
static int set(int16_t throttle) {
    uint8_t cmd[2];

    cmd[0] = (uint8_t) (throttle >> 8);
    cmd[1] = (uint8_t) throttle;

    return I2C_SoftWare_Master_Write(ESC_ADDR, ESC_REG_THROTTLE, sizeof(cmd), cmd) ? 1 : 0;
}

/**
 * Set throttle using internal range (0 - 32767) and using direction
 * @param pwm 0 - 32767
 */
static int set_internal(uint32_t throttle) {
    return set(((int16_t) throttle) * (direction == ESC_CLOCKWISE ? 1 : -1));
}

/**
 * Alternate function to set throttle using standard servo pulse range (1100-1900)
 * @param pwm Stopped - 1500 microseconds, Max forward - 1900 microseconds, Max reverse - 1100 microseconds
 */
static int setPWM_standard(uint32_t pwm) {
    return set(map(pwm, 1100, 1900, -32767, 32767));
}

/**
 * Set throttle using internal PWM range (0 - 1000) and using direction
 * @param pwm 0 - 1000
 */
static int setPWM_internal(uint32_t pwm) {
    current_PWM = pwm;
    return set(map(pwm, 0, 1000, 0, 32767) * (direction == ESC_CLOCKWISE ? 1 : -1));
}

/**
 * The update function reads new data from the ESC. If used, this function
 * must be called at least every 65 seconds to prevent 16-bit overflow of
 * the timer keeping track of RPM.
 * Called every 100 ms (0.1 seconds)
 * see http://docs.bluerobotics.com/bluesc/#rpm
 */
static void update(void) {
    buffer[8] = 0x00; // Reset last byte so we can check for alive

    readBuffer();

    _rpm = (buffer[0] << 8) | buffer[1];
    _identifier = buffer[8];

    /* 0.1 - delta time in seconds */
    _rpm = (int16_t) (((float) _rpm  * 60.0f) / (0.1f * (float) ESC_POLE_COUNT));
}

/**
 * @return true if the ESC is connected
 */
static bool isAlive() {
    return (_identifier == 0xAB);
}



/** *****************************************
*				Exported Functions - configuration							
****************************************** */

/**
 * \brief Initializes the ESC motor driver
 * @returns 0 - error, 1 - OK
 */
int ESC_init(void) {
    //init i2c bus
    I2C_SoftWare_Master_Init();
    ESC_SetDirection(ESC_CLOCKWISE);
    ESC_RPM_target = 0;
    current_PWM = 0;
    return ESC_powerOn();
}

/**
 *
 * @returns 0 - error, 1 - OK
 */
int ESC_deInit(void) {
    I2C_SoftWare_Master_Init();
    ESC_SetDirection(ESC_CLOCKWISE);
    ESC_RPM_target = 0;
    return ESC_powerOff();
}

/**
 * @brief  Stabilisation of ESC RPM
 *		   must be called every 100 ms
 */
uint8_t ESC_RPM_control(void) {
    static int timer = 0;
    int RPM_Act = 0;

    /* read from ESC controller and update current RPM */
    update();

    if (!isAlive()) {
        return 0;
    }

    /* request adjustments every 500 ms */
    if (timer++ >= 5) {
        timer = 0;
        RPM_Adjust_requested = true;
    }

    //If adjustment is requested and motor is turned on
    if (RPM_Adjust_requested && ESC_isPoweredOn()) {
        RPM_Adjust_requested = false;

        //Get actual RPM
        RPM_Act = ESC_getRPM();

        //Do not adjust if RPM speed is not requested or motor is not running
        if (ESC_RPM_target == 0) {
            if (RPM_Act > 0) {
                /* motor is moving, just keep current PWM */
                setPWM_internal(current_PWM);
            }
            return 0;
        }

        //adjustment required
        if (ESC_RPM_target != RPM_Act) {
            uint16_t Step;
            int Difference = 0;
            //Get current PWM parameter
            int PWM_Pulse = current_PWM;

            //Calculate RPM difference
            Difference = ESC_RPM_target - RPM_Act;

            //Select adjustment step, dependent on actual RPM difference
            if (abs(Difference) > 2000)
                Step = 100;
            else if (abs(Difference) > 100)
                Step = 10;
            else
                Step = 1;

            //Change PWM parameter if RPM can be adjusted more closely to target value. 
            if ((RPM_Act + ESC_RPM_ADJUSTMENT_STEP) < ESC_RPM_target) {
                //Actual RPM is lower than requested
                //Get max allowed PWM pulse length
                uint32_t MaxPossiblePulseLength = RPM2PWM(ESC_RPM_MAX);
                //Calculate new pulse length
                PWM_Pulse += Step;
                //Check for overflow
                if (PWM_Pulse > MaxPossiblePulseLength)
                    PWM_Pulse = MaxPossiblePulseLength;
                if (PWM_Pulse > ESC_PWM_MAX)
                    PWM_Pulse = ESC_PWM_MAX;
            } else if (RPM_Act > (ESC_RPM_target + ESC_RPM_ADJUSTMENT_STEP)) {
                //Actual RPM is higher than requested
                //ESC startup mode
#if 0
                if (ESC_Startup_Mode == true) {
                    // make sure this code runs for 2.5 seconds to smooth out motor start
                    loopCount++;
                    if (loopCount >= 5) {
                        loopCount = 0;
                        ESC_Startup_Mode = false; //Required RPM achieved, switch to normal mode
                    }

                    PWM_Pulse = RPM2PWM(ESC_RPM_target); //calculate requested PWM pulse length based on target RPM
                } else
#endif
                {
                    //ESC normal mode
                    uint32_t PWM_Pulse_Min = RPM2PWM(ESC_RPM_MIN); //Get min allowed PWM pulse length
                    PWM_Pulse -= Step; //Calculate new pulse length
                    PWM_Pulse_Min = ESC_PWM_MIN;
                    if (PWM_Pulse < PWM_Pulse_Min)
                        PWM_Pulse = PWM_Pulse_Min; //Motor will stop if pulse will be too low
                }
            }
            //Set new PWM parameter
            setPWM_internal((uint32_t) PWM_Pulse);
        } else {
            // send current value to keep motor moving
            setPWM_internal(current_PWM);
        }
    }
    return 1;
}

/** *****************************************
*				Exported Functions - operation
****************************************** */

/**
 * \brief Turns on power for ESC motor
 * @returns 0 - error, 1 - OK
 */
int ESC_powerOn(void) {
    ESC_powered_on = true;
    // must send a value of “0” at startup to initialize the thruster
    return set(0);
}

/**
 * \brief Turns off the power for ESC motor
 * @returns 0 - error, 1 - OK
 */
int ESC_powerOff(void) {
    ESC_RPM_target = 0;  //Request RPM = 0
    ESC_powered_on = false;
    // stop motor
    return set(0);
}

/**
 * \brief Get status of ESC motor power
 */
bool ESC_isPoweredOn(void) {
    return ESC_powered_on;
}

/**
 * \brief Sets the target RPM for ESC motor
 * \param rpm		The target RPM value
 *
 */
uint8_t ESC_setRPM(uint32_t rpm) {
    uint32_t pwm = 0;
    bool ESC_Startup_flag = false;

    //calculate new PWM pulse length for requested RPM
    pwm = RPM2PWM(rpm);

    /********* Startup mode *******
    *	ESC_STARTUP_PWM will be held until required RPM will be achieved
    *	Go into startup mode if startup PWM is higher than new required pwm and motor is not running
    *	Remark:	Reading ESC_getRPM() instead of ESC_RPM_target is not recommended because 
    *					actual RPM values might be falsified during ESC power switching
    */
    if (((ESC_Startup_PWM * 10) > pwm) && (ESC_RPM_target == 0) && (pwm != 0)) {
        //Set startup PWM pulse length
//        pwm = ESC_Startup_PWM * 10; //Internal PWM values are 0 - 1000
        //Set ESC startup flag for later processing
        ESC_Startup_flag = true;
    }

    //set new PWM
    if (setPWM_internal(pwm) != 0) {
        //New PWM was set successfully
        //store requested rpm globally for rpm adjustment routine
        ESC_RPM_target = rpm;
        //Set ESC mode
        ESC_Startup_Mode = ESC_Startup_flag;
    } else {
        //error occurred
        return 0;
    }

    return 1;
}

/**
 * \brief Sets the PWM width for ESC controller and starts it
 * \param rpm		The PWM width value 0 - 1000
 *
 */
uint8_t ESC_setPWM(uint32_t pwm) {
    ESC_RPM_target = 0;  //Exit RPM control mode to keep requested PWM
    setPWM_internal(pwm);
    return 1;
}

/**
*  \brief Returns the measured motor RPM
*	 \retval Motor period in RPM
 */
uint32_t ESC_getRPM(void) {
    return _rpm;
}

/**
 * \brief Gets the PWM duty cycle for ESC motor
 * \return pwm		The PWM duty cycle value of PWM timer
 *
 */
uint32_t ESC_getPWM() {
    return current_PWM;
}

/**
 * \brief Set direction of ESC motor
 */
uint8_t ESC_SetDirection(ESC_DirectionTypeDef Dir) {
    direction = Dir;
    return 1;
}

/**
 * \brief Get direction of ESC motor
 * \retval Direction in ESC_DirectionTypeDef type
 */
ESC_DirectionTypeDef ESC_GetDirection(void) {
    return direction;
}

/**
 * \brief Set StartupPWM of ESC motor.
 * \param pwm		The PWM width value (0 - 100). 0 - do not use StartupPWM function
 */
uint8_t ESC_setStartupPWM(uint32_t pwm) {
    if (pwm > 100)
        return 0;
    ESC_Startup_PWM = pwm;
    return 1;
}

/**
*  \brief Returns the StartupPWM value
*	 \retval StartupPWM value
 */
uint32_t ESC_getStartupPWM(void) {
    return ESC_Startup_PWM;
}


/**
 * \brief Set Slope of ESC motor.
 * \param slope
 */
uint8_t ESC_setSlope(uint32_t slope) {
    ESC_Slope = slope;
    return 1;
}

/**
*  \brief Returns the Slope value
*	 \retval Slope value
 */
uint32_t ESC_getSlope(void) {
    return ESC_Slope;
}

/**
 * \brief Set Intercept of ESC motor.
 * \param intercept
 */
uint8_t ESC_setIntercept(uint32_t intercept) {
    ESC_Intercept = intercept;
    return 1;
}

/**
*  \brief Returns the Intercept value
*	 \retval Intercept value
 */
uint32_t ESC_getIntercept(void) {
    return ESC_Intercept;
}
