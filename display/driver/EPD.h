#ifdef ALTRUIST_INSIGHT

#ifndef __EPD_H_ 
#define __EPD_H_ 

#ifdef DISPLAY_3IN52
#include "EPD_3in52.h"
#define DISPLAY_HEIGHT EPD_3IN52_WIDTH
#define DISPLAY_WIDTH EPD_3IN52_HEIGHT
#endif
#ifdef DISPLAY_4IN2
#include "EPD_4in2_SSD1683.h"
#define DISPLAY_WIDTH EPD_4IN2_V2_WIDTH
#define DISPLAY_HEIGHT EPD_4IN2_V2_HEIGHT
#endif

#endif


#endif
