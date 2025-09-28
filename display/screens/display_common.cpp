#ifdef ALTRUIST_INSIDE

#include "display_common.h"
#include "../paint_driver/GUI_Paint.h"

void initAndClearScreen() {
#ifdef DISPLAY_3IN52
    EPD_3IN52_Init();
    EPD_3IN52_display_NUM(EPD_3IN52_WHITE);
    EPD_3IN52_lut_GC();
    EPD_3IN52_refresh();
#endif
#ifdef DISPLAY_4IN2
    Paint_Clear(WHITE);
#endif
}

void createNewImage(UBYTE *&BlackImage) {
#ifdef DISPLAY_3IN52
    UWORD Imagesize = ((EPD_3IN52_WIDTH % 8 == 0)? (EPD_3IN52_WIDTH / 8 ): (EPD_3IN52_WIDTH / 8 + 1)) * EPD_3IN52_HEIGHT;
    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
    }
    Paint_NewImage(BlackImage, EPD_3IN52_WIDTH, EPD_3IN52_HEIGHT, 270, WHITE);
#endif
#ifdef DISPLAY_4IN2
    UWORD Imagesize = ((EPD_4IN2_V2_WIDTH % 8 == 0)? (EPD_4IN2_V2_WIDTH / 8 ): (EPD_4IN2_V2_WIDTH / 8 + 1)) * EPD_4IN2_V2_HEIGHT;
    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
    }
    Paint_NewImage(BlackImage, EPD_4IN2_V2_WIDTH, EPD_4IN2_V2_HEIGHT, 0, WHITE);
#endif
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
}

void showImageFast(UBYTE *&BlackImage) {
#ifdef DISPLAY_3IN52
    EPD_3IN52_SendCommand(0x50);
    EPD_3IN52_SendData(0x17);

    EPD_3IN52_display(BlackImage);
    EPD_3IN52_lut_DU();
    EPD_3IN52_refresh();
    DEV_Delay_ms(1000);
    printf("Goto Sleep...\r\n");
    EPD_3IN52_sleep();
#endif
#ifdef DISPLAY_4IN2
    EPD_4IN2_V2_Init();
    // EPD_4IN2_V2_Clear();
    DEV_Delay_ms(500);
    EPD_4IN2_V2_Display(BlackImage);
    DEV_Delay_ms(1000);
    EPD_4IN2_V2_Sleep();
    // DEV_Delay_ms(2000);
#endif
}

void showImageLong(UBYTE *&BlackImage) {
#ifdef DISPLAY_3IN52
    EPD_3IN52_SendCommand(0x50);
    EPD_3IN52_SendData(0x17);

    EPD_3IN52_display(BlackImage);
    EPD_3IN52_lut_GC();
    EPD_3IN52_refresh();
    DEV_Delay_ms(1000);
    printf("Goto Sleep...\r\n");
    EPD_3IN52_sleep();
#endif
#ifdef DISPLAY_4IN2
    EPD_4IN2_V2_Init();
    // EPD_4IN2_V2_Clear();
    DEV_Delay_ms(500);
    EPD_4IN2_V2_Display(BlackImage);
    DEV_Delay_ms(1000);
    EPD_4IN2_V2_Sleep();
    // DEV_Delay_ms(2000);
#endif
}

#endif