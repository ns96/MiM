
#include <Arduino.h>
#include "src/communication.h"
#include "src/Drivers/BLDC.h"
#include "src/Drivers/LED.h"
#include "src/Drivers/A4988.h"
#include "src/Drivers/LimitSW.h"
#include "src/Drivers/XY-KPWM.h"

unsigned long currentTime = 0, loopTime = 0;
int counter = 0; 
extern volatile uint16_t FGPeriod;
extern volatile uint16_t FGPeriod2;
extern volatile uint8_t BLDC_FG_HW_Mode;

void setup() {
  //Chinese boards are hard to program when the USB serial port is busy. delay helps to start programing the board without magic.
  //delay(3000);
  //Set clocksource of time Timer to clock/2
  //now Arduino time faster in 32 times
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
  //Serial1.println("start");
  
  BLDC_init();
  A4988_Init();
  LED_Init();
  LimitSW_Init();
  XY_KPWM_Init();
  // Turn on power LED
  LED_Set(LED_RED, LED_ON);

}

extern "C" void board_serial_print(char *line){
//  Serial.print(line);
  Serial1.print(line);    
}

void board_serial_println(String line){
 // Serial.println(line);
  Serial1.println(line);    
}

void serial_read(void){
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

  currentTime = millis();
  serial_read();
  
  if(currentTime < (loopTime + 100 * 32)) {
    return;
  }
  
  loopTime = currentTime;
  //create 1 sec events
  counter++;
  if (counter > 7) {
    counter = 1;
  }

  if (changeXYSpeed && XYEnabled) {
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
    Serial.println("XYPWM:"+(String)(XY_Speed));
    changeXYSpeed = false;  
  }
  //0.7 sec interval
  if (counter == 7) {
    XY_XPWM_Process();
    BLDC_FG_Process();
  }
  
  //Adjust BLDC motor speed
  BLDC_RPM_control();
  //Blink LEDs
  LED_Blinker();
}
