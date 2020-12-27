//China board use ATmega4808
//Arduino board use ATmega4809

#if defined (__AVR_ATmega4808__)
#include "board-4808.h"
#elif defined (__AVR_ATmega4809__)
#include "board-4809.h"
#endif
