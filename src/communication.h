#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_
#include "Commands.h"
#ifdef __cplusplus
extern "C" {
#endif
void comm_receivedByte(uint8_t received);
void communication_callback(void);
#ifdef __cplusplus
}
#endif
#endif

