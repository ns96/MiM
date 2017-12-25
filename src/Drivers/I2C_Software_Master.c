#include <stdbool.h>
#include <stm32f0xx_conf.h>
#include "I2C_SoftWare_Master.h"
#include "main.h"

// https://github.com/multiwii/baseflight/blob/master/src/drv_i2c_soft.c

#define SCL_H         I2C_GPIO->BSRR = GPIO_Pin_SCL
#define SCL_L         I2C_GPIO->BRR = GPIO_Pin_SCL

#define SDA_H         I2C_GPIO->BSRR = GPIO_Pin_SDA
#define SDA_L         I2C_GPIO->BRR = GPIO_Pin_SDA

#define SCL_read      I2C_GPIO->IDR & GPIO_Pin_SCL
#define SDA_read      I2C_GPIO->IDR & GPIO_Pin_SDA

#define I2C_DIRECTION_TRANSMITTER       ((uint8_t)0x00)
#define I2C_DIRECTION_RECEIVER          ((uint8_t)0x01)

void I2C_SoftWare_Master_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    /*SDA GPIO clock enable */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);

    /*SCL GPIO clock enable */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);

    /* GPIO Configuration */
    /* Configure I2C SCL pin */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_SCL;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Configure I2C SDA pin */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_SDA;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    SCL_H;
    SDA_H;

    SCL_L;
    SDA_L;

    SCL_H;
    SDA_H;
}

/**
 * @brief  Delays for amount of micro seconds
 * https://github.com/MaJerle/stm32fxxx_hal_libraries/blob/master/00-STM32_LIBRARIES/tm_stm32_delay.h
 * @param  micros: Number of microseconds for delay
 * @retval None
 */
__STATIC_INLINE void Delay(__IO uint32_t micros) {
    /* Go to clock cycles */
	micros *= (SystemCoreClock / 1000000) / 5;

	/* Wait till done */
	while (micros--);
}


void I2C_delay(void) {
    Delay(10);
}

static bool I2C_Start(void)
{
    SDA_H;
    SCL_H;
    I2C_delay();
    if (!SDA_read)
        return false;
    SDA_L;
    I2C_delay();
    if (SDA_read)
        return false;
    SDA_L;
    I2C_delay();
    return true;
}

static void I2C_Stop(void)
{
    SCL_L;
    I2C_delay();
    SDA_L;
    I2C_delay();
    SCL_H;
    I2C_delay();
    SDA_H;
    I2C_delay();
}


static void I2C_Ack(void)
{
    SCL_L;
    I2C_delay();
    SDA_L;
    I2C_delay();
    SCL_H;
    I2C_delay();
    SCL_L;
    I2C_delay();
}

static void I2C_NoAck(void)
{
    SCL_L;
    I2C_delay();
    SDA_H;
    I2C_delay();
    SCL_H;
    I2C_delay();
    SCL_L;
    I2C_delay();
}

static bool I2C_WaitAck(void)
{
    SCL_L;
    I2C_delay();
    SDA_H;
    I2C_delay();
    SCL_H;

    I2C_delay();
    if (SDA_read) {
        SCL_L;
        return false;
    }
    SCL_L;
    return true;
}

static void I2C_SendByte(uint8_t byte)
{
    uint8_t i = 8;
    while (i--) {
        SCL_L;
        I2C_delay();
        if (byte & 0x80)
            SDA_H;
        else
            SDA_L;
        byte <<= 1;
        I2C_delay();
        SCL_H;
        I2C_delay();
    }
    SCL_L;
}

static uint8_t I2C_ReceiveByte(void)
{
    uint8_t i = 8;
    uint8_t byte = 0;

    SDA_H;
    while (i--) {
        byte <<= 1;
        SCL_L;
        I2C_delay();
        SCL_H;
        I2C_delay();
        if (SDA_read) {
            byte |= 0x01;
        }
    }
    SCL_L;
    return byte;
}

static bool I2C_SoftWare_Master_ReInit(void)
{
    I2C_SoftWare_Master_Init();

    return false;
}

/**
 *
 * @param addr
 * @param reg
 * @param len
 * @param buf
 * @return true - OK, false - ERROR
 */
bool I2C_SoftWare_Master_Write(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *data)
{
    int i;
    if (!I2C_Start())
        return I2C_SoftWare_Master_ReInit();
    I2C_SendByte((addr << 1) | I2C_DIRECTION_TRANSMITTER);
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return I2C_SoftWare_Master_ReInit();
    }
    I2C_SendByte(reg);
    I2C_WaitAck();
    for (i = 0; i < len; i++) {
        I2C_SendByte(data[i]);
        if (!I2C_WaitAck()) {
            I2C_Stop();
            return I2C_SoftWare_Master_ReInit();
        }
    }
    I2C_Stop();
    return true;
}

/**
 *
 * @param addr
 * @param reg
 * @param len
 * @param buf
 * @return true - OK, false - ERROR
 */
bool I2C_SoftWare_Master_Read(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
    if (!I2C_Start())
        return I2C_SoftWare_Master_ReInit();
    I2C_SendByte((addr << 1) | I2C_DIRECTION_TRANSMITTER);
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return false;
    }
    I2C_SendByte(reg);
    I2C_WaitAck();
    I2C_Start();
    I2C_SendByte((addr << 1) | I2C_DIRECTION_RECEIVER);
    I2C_WaitAck();
    while (len) {
        *buf = I2C_ReceiveByte();
        if (len == 1)
            I2C_NoAck();
        else
            I2C_Ack();
        buf++;
        len--;
    }
    I2C_Stop();
    return true;
}
