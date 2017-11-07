#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_
#include "stm32f0xx.h"
#include "Commands.h"
void comm_receivedByte(uint8_t received);
void communication_callback(void);
#endif

