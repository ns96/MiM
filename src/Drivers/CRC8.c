/*  
 * crc8.c
 * 
 * Computes a 8-bit CRC 
 * 
 */

#include <stdio.h>
#include <stdint.h>


#define GP  0x107   /* x^8 + x^2 + x + 1 */
#define DI  0x07


static uint8_t crc8_table[256];     /* 8-bit table */
static uint8_t made_table=0;

/*
* Init CRC8 table 
*/
static void init_crc8(void)
{
  uint16_t i, j;
  uint8_t crc;

  if (!made_table) {
    for (i=0; i<256; i++) {
      crc = i;
      for (j=0; j<8; j++)
        crc = (crc << 1) ^ ((crc & 0x80) ? DI : 0);
      crc8_table[i] = crc & 0xFF;
      //printf("table[%d] = %d (0x%X)\n", i, crc, crc);
    }
    made_table=1;
  }
}


 /*
	* For a byte array whose accumulated crc value is stored in *crc, computes
	* resultant crc obtained by appending m to the byte array
	*/
void crc8(uint8_t *crc, uint8_t m)
{
  if (!made_table)
    init_crc8();

  *crc = crc8_table[(*crc) ^ m];
  *crc &= 0xFF;
}
