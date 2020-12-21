
#include <Arduino.h>
#include "src/communication.h"
#include "src/Drivers/BLDC.h"
#include "src/Drivers/LED.h"
#include "src/Drivers/A4988.h"
#include "src/Drivers/LimitSW.h"
#include "src/Drivers/XY-KPWM.h"

unsigned long currentTime = 0, loopTime = 0;


void setup() {
  //Chinese boards are hard to program when the USB serial port is busy. delay helps to start programing the board without magic.
  delay(3000);
    //Initialize the peripherals
  //Serial.begin(19200);
  Serial1.begin(19200);
  Serial1.println("start");
  BLDC_init();
  A4988_Init();
  LED_Init();
  LimitSW_Init();
  XY_KPWM_Init();
  // Turn on power LED
  LED_Set(LED_RED, LED_ON);
//  analogWriteFrequency(32);
}

extern "C" void board_serial_print(char *line){
  Serial.print(line);
  Serial1.print(line);    
}

void board_serial_println(String line){
  Serial.println(line);
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
  
  if(currentTime < (loopTime + 100)) {
    return;
  }
  //BOARD_SERIAL.println("Time:"+(String)(currentTime));
  loopTime = currentTime;
  if (changePWMSpeed) {
    board_serial_println("XYPWM:"+(String)(pwmSpeed));  
  }
  //BOARD_SERIAL.println("XYPWM:"+(String)(pwmSpeed)); 
  //Adjust BLDC motor speed
  //BLDC_RPM_control();
  //Blink LEDs
  LED_Blinker();
}
