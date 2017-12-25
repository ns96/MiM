#ifndef _I2C_H_
#define _I2C_H_

#include "stm32f0xx.h"
#include "board.h"


void I2C_SoftWare_Master_Init(void);

bool I2C_SoftWare_Master_Write(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *data);

bool I2C_SoftWare_Master_Read(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf);


#endif
