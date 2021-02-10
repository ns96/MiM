/**
 * MiM firmware for Arduino Nano Every board.
 */

#include <Arduino.h>
#include "src/communication.h"
#include "src/Drivers/BLDC.h"
#include "src/Drivers/LED.h"
#include "src/Drivers/A4988.h"
#include "src/Drivers/LimitSW.h"
#include "src/Drivers/XY-KPWM.h"

// Arduino time (eg millis()) runs faster
#define MILLIS_FACTOR   (32)

/**
 * Blink LED_BUILTIN every second
 */
static void blink_builtin_led() {
  static unsigned long time = millis();
  static int blink_count = 0;

  if (millis() - time >= 1000 * MILLIS_FACTOR) {
    digitalWrite(LED_BUILTIN, blink_count & 0x1 ? HIGH : LOW);

    time = millis();
    blink_count++;
  }
}

void setup() {
  //Chinese boards are hard to program when the USB serial port is busy. delay helps to start programing the board without magic.
  //delay(3000);
  //Set clocksource of time Timer to clock/2
  //now Arduino time faster in 32 times (MILLIS_FACTOR)
  #if defined(MILLIS_USE_TIMERB2) 
  TCB2.CTRLA = TCB_CLKSEL_CLKDIV2_gc  /* CLK_PER/2 (From Prescaler) */
               | 1 << TCB_ENABLE_bp;   /* Enable: enabled */
  #elif defined(MILLIS_USE_TIMERB3)
  TCB3.CTRLA = TCB_CLKSEL_CLKDIV2_gc  /* CLK_PER/2 (From Prescaler) */
               | 1 << TCB_ENABLE_bp;   /* Enable: enabled */
  #endif
  Serial.begin(19200);
  Serial1.setTimeout(125);
  Serial1.begin(19200);
  Serial.println("MiM-nano version 1.0.3");
  
  BLDC_init();
  A4988_Init();
  LED_Init();
  LimitSW_Init();
  XY_KPWM_Init();
  // Turn on power LED
  LED_Set(LED_RED, LED_ON);
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
}

extern "C" void board_serial_print(char *line){
  Serial.print(line);
  Serial1.print(line);    
}

void board_serial_println(String line){
  Serial.println(line);
  Serial1.println(line);    
}

static void serial_read() {
  // Parse and execute commands received over UART
  while (Serial.available()) {
    comm_receivedByte(Serial.read());
    communication_callback();  
  }
  while (Serial1.available()) {
    comm_receivedByte(Serial1.read());
    communication_callback();  
  }   
}

void loop() {
  static unsigned long currentTime = 0, loopTime = 0;
  static int counter = 0;

  currentTime = millis();
  serial_read();

  blink_builtin_led();

  // 100ms interval
  if (currentTime < (loopTime + 100 * MILLIS_FACTOR)) {
    return;
  }
  
  loopTime = currentTime;
  //create 0.7 sec events
  counter++;
  if (counter > 7) {
    counter = 1;
  }

  if (changeXYSpeed && XYEnabled) {
    Serial.println("XYPWM:" + (String)(XY_Speed));
    if (XY_Speed > 0) {
      //enable BLDC
      if (!BLDC_getPower()) {
        BLDC_powerOn();
        Serial.println("BLDC on");
      }
      //set RPM
      BLDC_setRPM(XY_Speed);
      Serial.println("BLDC set RPM:" + (String)(XY_Speed));
    } else {
      //XY disabled. turnoff
      if (BLDC_getPower()) {
        BLDC_powerOff();
        Serial.println("BLDC off");
      }
    }
    changeXYSpeed = false;
  }
  //0.7 sec interval
  if (counter == 7) {
    XY_XPWM_Process();
  }
  
  //Adjust BLDC motor speed
  BLDC_RPM_control();
  //Blink LEDs
  LED_Blinker();
}
