#ifdef ALTRUIST_INSIGHT

#ifndef DISPLAY_COMMON_H
#define DISPLAY_COMMON_H

#include "../driver/EPD.h"
#include "../paint_driver/GUI_Paint.h"
#include "../display_modes.h"

enum class ScreenPage;

void initAndClearScreen();
void createNewImage(UBYTE *&BlackImage);
void showImageFast(UBYTE *&BlackImage, ScreenPage currentScreen);
void showImageLong(UBYTE *&BlackImage);
void drawScreenIndicator(ScreenPage currentScreen);

// High-level API for working with display modes and update counter.
// Lifecycle: init -> display (calls turnOn inside) -> sleep.

// Initialize display in specified mode (FULL/FAST/PARTIAL/GRAY_4).
void epdInit(DisplayMode mode);

// Display image in specified mode (full, fast, partial, 4-gray).
// Returns true on success, false if display was stuck (recovery attempted automatically).
bool epdDisplay(DisplayMode mode, UBYTE *Image);

// Put display to sleep.
void epdSleep();

// Manually increment update counter (for direct driver access).
void epdIncrementUpdateCount();

// Get number of screen updates since startup.
unsigned long epdGetUpdateCount();

// Reset display state (for wake from sleep, like on first power-on)
void epdResetState();

// Reset period position counter (for full refresh after wake)
void epdResetPeriodPosition();

// Increment counter for a specific screen (called on button navigation)
void epdIncrementScreenCounter(ScreenPage screen);

// Set display state (for wake from sleep)
void epdSetInitialized(bool initialized, DisplayMode mode);

// Attempt to recover from a stuck display (hardware reset + reinit)
void epdRecoverFromStuck();

/** True when cfg allows partial e-paper updates (experimental mode). */
bool epdPartialRefreshEnabled();

// Draw rounded rectangle (for sensors map etc.)
void Paint_DrawRoundedRectangle(UWORD xStart, UWORD yStart, UWORD xEnd, UWORD yEnd,
                                UWORD color, UWORD radius, DOT_PIXEL line_width, DRAW_FILL draw_fill);

#ifdef DISPLAY_4IN2
#define EPD_DisplayFull(img)   EPD_4IN2_V2_Display(img)
#define EPD_DisplayPartial(img,xs,ys,xe,ye)  EPD_4IN2_V2_PartialDisplay(img,xs,ys,xe,ye)
#endif

#endif
#endif