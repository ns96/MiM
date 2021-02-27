//China board use ATmega4808
//Arduino board use ATmega4809

// uncomment to use alternate pin routing for Keyestudio CNC Shield
//#define USEKEYESTUDIO_BOARD

#if defined (USEKEYESTUDIO_BOARD)
#include "board-4809-keyestudio.h"
#elif defined (__AVR_ATmega4808__)
#include "board-4808.h"
#elif defined (__AVR_ATmega4809__)
#include "board-4809.h"
#endif
