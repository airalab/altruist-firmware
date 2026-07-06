#ifdef ALTRUIST_INSIGHT

#include "display_common.h"
#include "../paint_driver/GUI_Paint.h"
#include "../display_manager.h"
// Icon sets
#include "../icons/icons/icons_10x10.h"
#include "../icons/icons/icons_15x15.h"
#include "../display_modes.h"
#include "../../defines.h"
#include "../../config_manager/config_defaults.h"

static bool epd_initialized = false;
static DisplayMode epd_current_mode = DisplayMode::FULL; // Track current mode to avoid unnecessary re-init
static unsigned long epd_update_count = 0;
static unsigned int initial_full_refreshes_done = 0; // Track first 3 full refreshes after boot
static ScreenPage last_screen = ScreenPage::MAIN; // Track last screen to detect screen changes
// Track period position per screen (indexed by ScreenPage enum value)
static unsigned int period_position_per_screen[9] = {0}; // Max 9 screens, all initialized to 0
// Global counter for non-MAIN screen switches (shared across all non-MAIN screens)
static unsigned int non_main_screen_switch_counter = 0;

bool epdPartialRefreshEnabled() {
	return cfg::epd_refresh_mode == EPD_REFRESH_EXPERIMENTAL_PARTIAL;
}

// Helper: Get safe screen index
static unsigned int getScreenIndex(ScreenPage screen) {
    unsigned int idx = static_cast<unsigned int>(screen);
    return (idx >= 9) ? 0 : idx;
}

// Helper: Get refresh period for a screen
static unsigned int getRefreshPeriod(ScreenPage screen) {
    return (screen == ScreenPage::MAIN) ? 11 : 6;
}

// Helper: Reset counter after full refresh
static void resetCounterAfterFullRefresh(ScreenPage screen) {
    if (screen == ScreenPage::MAIN) {
        period_position_per_screen[getScreenIndex(screen)] = 0;
    } else {
        non_main_screen_switch_counter = 0;
    }
}

// Helper: Get current period position for a screen
static unsigned int getPeriodPosition(ScreenPage screen) {
    if (screen == ScreenPage::MAIN) {
        return period_position_per_screen[getScreenIndex(screen)];
    } else {
        return non_main_screen_switch_counter;
    }
}

// Helper function: increment global update counter
void epdIncrementUpdateCount() {
    epd_update_count++;
}

unsigned long epdGetUpdateCount() {
    return epd_update_count;
}

void initAndClearScreen() {
#ifdef DISPLAY_4IN2
    // Simplified initialization: only on first startup, without unnecessary clears
    if (!epd_initialized) {
        // Use fast initialization even for first startup (faster)
        EPD_4IN2_V2_Init_Fast(Seconds_1S);
        epd_initialized = true;
        epd_current_mode = DisplayMode::FAST; // Will be switched on first use
    }
#endif
}

// Unified init by mode.
// Optimized: re-initialization only when mode changes.
void epdInit(DisplayMode mode) {
#ifdef DISPLAY_4IN2
    // Re-initialize only if mode changed or display is not initialized
    if (!epd_initialized || epd_current_mode != mode) {
        switch (mode) {
            case DisplayMode::FULL:
                // For "full" update use fast init (faster, but still clean)
                EPD_4IN2_V2_Init_Fast(Seconds_1S);
                break;
            case DisplayMode::FAST:
            case DisplayMode::PARTIAL:
                // For fast and partial updates use fast init.
                EPD_4IN2_V2_Init_Fast(Seconds_1S);
                break;
            case DisplayMode::GRAY_4:
                EPD_4IN2_V2_Init_4Gray();
                break;
        }
        epd_current_mode = mode;
        epd_initialized = true;
    }
#else
    (void)mode;
#endif
}

void createNewImage(UBYTE *&BlackImage) {
#ifdef DISPLAY_4IN2
    UWORD Imagesize = ((EPD_4IN2_V2_WIDTH % 8 == 0)? (EPD_4IN2_V2_WIDTH / 8 ): (EPD_4IN2_V2_WIDTH / 8 + 1)) * EPD_4IN2_V2_HEIGHT;
    if (BlackImage) free(BlackImage);
    BlackImage = (UBYTE *)malloc(Imagesize);
    if (BlackImage == NULL) {
        return;
    }
    Paint_NewImage(BlackImage, EPD_4IN2_V2_WIDTH, EPD_4IN2_V2_HEIGHT, ROTATE_0, WHITE);
    Paint_Clear(WHITE);
#endif
}

void showImageFast(UBYTE *&BlackImage, ScreenPage currentScreen) {
#ifdef DISPLAY_4IN2
    // Fast update without going to sleep.
    // Update logic:
    // 1. First 3 updates after boot - always full (for clean initialization)
    // 2. On MAIN screen (1-minute data updates): 10 partial, then 1 full
    // 3. When changing pages: 5 partial, then 1 full
    
    // First 3 updates - always full
    if (initial_full_refreshes_done < 3) {
        epdDisplay(DisplayMode::FULL, BlackImage);
        initial_full_refreshes_done++;
        // Reset all screen counters after initial full refreshes
        for (int i = 0; i < 9; i++) {
            period_position_per_screen[i] = 0;
        }
        last_screen = currentScreen;
        return;
    }
    
    bool is_main_screen = (currentScreen == ScreenPage::MAIN);
    unsigned int full_refresh_period = getRefreshPeriod(currentScreen);
    unsigned int period_position = getPeriodPosition(currentScreen);
    
    // Detect screen change
    bool screen_changed = (currentScreen != last_screen);
    
    if (screen_changed) {
        last_screen = currentScreen;
        
        // Counter was already incremented by epdIncrementScreenCounter() when button was pressed
        // Check if we need a full refresh
        if (period_position >= full_refresh_period) {
            epdDisplay(DisplayMode::FULL, BlackImage);
            resetCounterAfterFullRefresh(currentScreen);
            return;
        }
        // Otherwise continue with partial refresh below
    } else {
        // Same screen refresh - only increment for MAIN screen (timer-based updates)
        // For other screens, refreshes only happen on button press (already incremented)
        if (is_main_screen) {
            period_position_per_screen[getScreenIndex(currentScreen)]++;
            period_position = getPeriodPosition(currentScreen);
        }
        // For non-MAIN screens, counter was already incremented by button press, don't increment again
    }
    
    // Check if we need a full refresh (at the end of the period)
    bool do_full_refresh = (period_position >= full_refresh_period);
    
    if (do_full_refresh) {
        epdDisplay(DisplayMode::FULL, BlackImage);
        resetCounterAfterFullRefresh(currentScreen);
    } else if (epdPartialRefreshEnabled()) {
        epdDisplay(DisplayMode::PARTIAL, BlackImage);
    } else {
        epdDisplay(DisplayMode::FULL, BlackImage);
        resetCounterAfterFullRefresh(currentScreen);
    }
#endif
}

void showImageLong(UBYTE *&BlackImage) {
#ifdef DISPLAY_4IN2
    // Full "long" update (e.g., after wake or for rare static screens).
    // Use original full init for maximum reliability and cleanliness.
    EPD_4IN2_V2_Init();  // Use original full init (not fast)
    EPD_4IN2_V2_Display(BlackImage);
    DEV_Delay_ms(100);
    epd_initialized = true;  // Update flag so epdInit knows display is initialized
    epd_current_mode = DisplayMode::FULL;  // Update current mode
    epdIncrementUpdateCount();
    // Reset all screen counters after full refresh
    for (int i = 0; i < 9; i++) {
        period_position_per_screen[i] = 0;
    }
#endif
}

// High-level display function by mode.
// Returns true on success, false if display was stuck and recovery was attempted.
bool epdDisplay(DisplayMode mode, UBYTE *Image) {
#ifdef DISPLAY_4IN2
    epdInit(mode);

    bool ok = true;
    switch (mode) {
        case DisplayMode::FULL:
            ok = EPD_4IN2_V2_Display_Fast(Image);
            break;
        case DisplayMode::FAST:
            ok = EPD_4IN2_V2_Display_Fast(Image);
            break;
        case DisplayMode::PARTIAL:
            ok = EPD_4IN2_V2_PartialDisplay(Image);
            break;
        case DisplayMode::GRAY_4:
            ok = EPD_4IN2_V2_Display_4Gray(Image);
            break;
    }

    epdIncrementUpdateCount();

    if (!ok) {
        Serial.println(F("[EPD] Display stuck detected — recovering and retrying with FULL refresh"));
        epdRecoverFromStuck();
        epdInit(DisplayMode::FULL);
        EPD_4IN2_V2_Display_Fast(Image);
        epdIncrementUpdateCount();
        for (int i = 0; i < 9; i++) {
            period_position_per_screen[i] = 0;
        }
        non_main_screen_switch_counter = 0;
        return false;
    }

    return true;
#else
    (void)mode;
    (void)Image;
    return true;
#endif
}

void epdSleep() {
#ifdef DISPLAY_4IN2
    EPD_4IN2_V2_Sleep();
    epd_initialized = false; // Reset flag so wake-up gets full initialization
#endif
}

// Attempt to recover from a stuck display by hardware reset.
// Leaves epd_initialized = false so the next epdInit() call performs
// a proper mode-specific initialization before any display operation.
void epdRecoverFromStuck() {
#ifdef DISPLAY_4IN2
#if defined(ALTRUIST_BUILD_DEBUG)
    Serial.println(F("[EPD] Attempting display recovery (hardware reset)..."));
#endif
    
    EPD_4IN2_V2_Reset();
    DEV_Delay_ms(100);
    
    epd_initialized = false;
    epd_current_mode = DisplayMode::FULL;
    
    for (int i = 0; i < 9; i++) {
        period_position_per_screen[i] = 0;
    }
    
#if defined(ALTRUIST_BUILD_DEBUG)
    Serial.println(F("[EPD] Hardware reset complete, awaiting re-init"));
#endif
#endif
}

void epdResetState() {
#ifdef DISPLAY_4IN2
    // Reset display state (like on first power-on)
    epd_initialized = false;
    epd_current_mode = DisplayMode::FULL;
    // Reset all screen counters on wake/reset
    for (int i = 0; i < 9; i++) {
        period_position_per_screen[i] = 0;
    }
    last_screen = ScreenPage::MAIN; // Reset screen tracking
    // Don't reset update counter - it continues counting
#endif
}

void epdResetPeriodPosition() {
#ifdef DISPLAY_4IN2
    // Reset all screen counters
    for (int i = 0; i < 9; i++) {
        period_position_per_screen[i] = 0;
    }
#endif
}

void epdIncrementScreenCounter(ScreenPage screen) {
#ifdef DISPLAY_4IN2
    bool is_main_screen = (screen == ScreenPage::MAIN);
    unsigned int full_refresh_period = getRefreshPeriod(screen);
    
    if (is_main_screen) {
        // MAIN screen: use per-screen counter
        unsigned int &period_position = period_position_per_screen[getScreenIndex(screen)];
        
        if (period_position >= full_refresh_period) {
            period_position = 0;
        }
        period_position++;
    } else {
        // Non-MAIN screens: use shared global counter for all screen switches
        // If counter is already at or past limit, reset it first
        if (non_main_screen_switch_counter >= full_refresh_period) {
            non_main_screen_switch_counter = 0;
        }
        
        // Increment shared global counter for non-MAIN screens
        non_main_screen_switch_counter++;
    }
#endif
}

void epdSetInitialized(bool initialized, DisplayMode mode) {
#ifdef DISPLAY_4IN2
    epd_initialized = initialized;
    epd_current_mode = mode;
#endif
}

// Helper function to draw an icon with inverted colors (white icon on black background)
static void Paint_DrawImageInverted(const unsigned char *image_buffer, UWORD xStart, UWORD yStart, UWORD W_Image, UWORD H_Image) {
    UWORD x, y;
    UWORD byte_width = (W_Image % 8) ? (W_Image / 8 + 1) : (W_Image / 8);

    for (y = 0; y < H_Image; y++) {
        for (x = 0; x < W_Image; x++) {
            UWORD byte_index = (y * byte_width) + (x / 8);
            UBYTE byte = image_buffer[byte_index];
            UBYTE bit = 0x80 >> (x % 8);  // MSB first

            // Invert: if bit is set (would be WHITE), draw BLACK; if bit is clear (would be BLACK), draw WHITE
            UWORD color = (byte & bit) ? BLACK : WHITE;

            Paint_SetPixel(xStart + x, yStart + y, color);
        }
    }
}

// Draw rounded rectangle (exposed for sensors map etc.)
void Paint_DrawRoundedRectangle(UWORD xStart, UWORD yStart, UWORD xEnd, UWORD yEnd, 
                                        UWORD color, UWORD radius, DOT_PIXEL line_width, DRAW_FILL draw_fill) {
    if (xStart > Paint.Width || yStart > Paint.Height ||
        xEnd > Paint.Width || yEnd > Paint.Height) {
        return;
    }

    if (draw_fill == DRAW_FILL_FULL) {
        // Fill the main rectangular area
        Paint_DrawRectangle(xStart + radius, yStart, xEnd - radius, yEnd, color, line_width, DRAW_FILL_FULL);
        Paint_DrawRectangle(xStart, yStart + radius, xEnd, yEnd - radius, color, line_width, DRAW_FILL_FULL);
        
        // Fill the corner areas with filled circles (only the parts that are inside the rectangle)
        // Top-left
        for (int y = 0; y < radius; y++) {
            for (int x = 0; x < radius; x++) {
                int dx = x - radius;
                int dy = y - radius;
                if (dx*dx + dy*dy <= radius*radius) {
                    Paint_SetPixel(xStart + x, yStart + y, color);
                }
            }
        }
        // Top-right
        for (int y = 0; y < radius; y++) {
            for (int x = 0; x < radius; x++) {
                int dx = x;
                int dy = y - radius;
                if (dx*dx + dy*dy <= radius*radius) {
                    Paint_SetPixel(xEnd - radius + x, yStart + y, color);
                }
            }
        }
        // Bottom-left
        for (int y = 0; y < radius; y++) {
            for (int x = 0; x < radius; x++) {
                int dx = x - radius;
                int dy = y;
                if (dx*dx + dy*dy <= radius*radius) {
                    Paint_SetPixel(xStart + x, yEnd - radius + y, color);
                }
            }
        }
        // Bottom-right
        for (int y = 0; y < radius; y++) {
            for (int x = 0; x < radius; x++) {
                int dx = x;
                int dy = y;
                if (dx*dx + dy*dy <= radius*radius) {
                    Paint_SetPixel(xEnd - radius + x, yEnd - radius + y, color);
                }
            }
        }
    } else {
        // Draw outline - for small radius, use simple rectangle with rounded corners
        // Top and bottom horizontal lines
        Paint_DrawLine(xStart + radius, yStart, xEnd - radius, yStart, color, line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(xStart + radius, yEnd, xEnd - radius, yEnd, color, line_width, LINE_STYLE_SOLID);
        // Left and right vertical lines
        Paint_DrawLine(xStart, yStart + radius, xStart, yEnd - radius, color, line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(xEnd, yStart + radius, xEnd, yEnd - radius, color, line_width, LINE_STYLE_SOLID);
        
        // Draw corner arcs using pixel-by-pixel approach
        // Top-left corner
        for (int y = 0; y < radius; y++) {
            for (int x = 0; x < radius; x++) {
                int dx = x - radius;
                int dy = y - radius;
                int dist_sq = dx*dx + dy*dy;
                if (dist_sq >= (radius-1)*(radius-1) && dist_sq <= radius*radius) {
                    Paint_SetPixel(xStart + x, yStart + y, color);
                }
            }
        }
        // Top-right corner
        for (int y = 0; y < radius; y++) {
            for (int x = 0; x < radius; x++) {
                int dx = x;
                int dy = y - radius;
                int dist_sq = dx*dx + dy*dy;
                if (dist_sq >= (radius-1)*(radius-1) && dist_sq <= radius*radius) {
                    Paint_SetPixel(xEnd - radius + x, yStart + y, color);
                }
            }
        }
        // Bottom-left corner
        for (int y = 0; y < radius; y++) {
            for (int x = 0; x < radius; x++) {
                int dx = x - radius;
                int dy = y;
                int dist_sq = dx*dx + dy*dy;
                if (dist_sq >= (radius-1)*(radius-1) && dist_sq <= radius*radius) {
                    Paint_SetPixel(xStart + x, yEnd - radius + y, color);
                }
            }
        }
        // Bottom-right corner
        for (int y = 0; y < radius; y++) {
            for (int x = 0; x < radius; x++) {
                int dx = x;
                int dy = y;
                int dist_sq = dx*dx + dy*dy;
                if (dist_sq >= (radius-1)*(radius-1) && dist_sq <= radius*radius) {
                    Paint_SetPixel(xEnd - radius + x, yEnd - radius + y, color);
                }
            }
        }
    }
}

// Draw screen indicator icons as a vertical stack on the right side
void drawScreenIndicator(ScreenPage currentScreen) {
    bool isNavigable = (currentScreen == ScreenPage::MAIN || 
                       currentScreen == ScreenPage::ANALYTICS ||
                       currentScreen == ScreenPage::GRAPHS || 
                       currentScreen == ScreenPage::SETTINGS || 
                       currentScreen == ScreenPage::SENSOR_MAP);
    if (!isNavigable) {
        return;
    }

    struct NavIcon {
        const unsigned char* icon;
        bool isPageIcon;
        ScreenPage screen; // Only used for page icons
    };


    // Navigation items
    NavIcon navItems[6] = {
        {home_nav_15x15,    true,  ScreenPage::MAIN},
        {chart_pie_15x15,   true,  ScreenPage::ANALYTICS},
        {chart_15x15,       true,  ScreenPage::GRAPHS},      // chart icon for graphs
        {map_nav_15x15,     true,  ScreenPage::SENSOR_MAP},
        {settings_15x15,    true,  ScreenPage::SETTINGS},    // settings icon for settings page
        {new_switch_15x15,  false, ScreenPage::MAIN}         // bottom switch icon
    };

    const int icon_size = 10; // small icon size
    const int large_icon_size = 15; // large icon size for page icons
    const int icon_spacing = 4; // vertical spacing between icons
    const int nav_item_count = 6;
    const int sidebar_width = 26; // width of the navigation sidebar 
    const int button_height = large_icon_size + 8; // icon + vertical padding
    const int border_radius = 0; 
    const int padding = 4; // padding inside the rounded rectangle
    const int margin = 0; // margin from right and bottom edges of screen 
    
    // Reserve space at the top for the main header (icon/time/date) + its bottom border.
    // Main header bottom line is around y=26; start sidebar at y=27 so it sits just under it.
    const int header_reserved_height = 27;

    // Calculate sidebar position (right side of screen, starting below header)
    int sidebar_x = DISPLAY_WIDTH - sidebar_width - margin;
    int sidebar_y = header_reserved_height;
    int sidebar_height = DISPLAY_HEIGHT - header_reserved_height - margin;


    // Sidebar background (no top border so it blends with header)
    Paint_DrawRectangle(sidebar_x, sidebar_y, 
                        sidebar_x + sidebar_width, sidebar_y + sidebar_height - 1,
                        WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    // Left
    Paint_DrawLine(sidebar_x, sidebar_y, sidebar_x, sidebar_y + sidebar_height - 1,
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    // Calculate icon positions
    int small_icon_x = sidebar_x + (sidebar_width - icon_size) / 2;
    int large_icon_x = sidebar_x + (sidebar_width - large_icon_size) / 2;
    const int page_icon_gap = 8; 
    

    // Extra top padding so the first icon sits a bit lower
    int current_y = sidebar_y + 10; 
    
    for (int i = 0; i <= 4; i++) {
        bool is_active = (navItems[i].screen == currentScreen);
        
        // Page icons are all 15x15
        bool is_large_icon = true;
        int current_icon_size = is_large_icon ? large_icon_size : icon_size;
        int current_icon_x = is_large_icon ? large_icon_x : small_icon_x;
        
        // Draw black button background for active page
        if (is_active) {
            const int top_padding    = 4;  // space above icon 
            const int bottom_padding = 8;  // space below icon
            int button_x = sidebar_x;      // stick to the right/left edges of sidebar

            int button_y;
            int button_h;
            
            if (i == 0) {
                // First icon (home): extend all the way to header,
                // and give it a bit more bottom padding so the icon feels vertically centered.
                const int extra_bottom_padding = 4; // additional space only for home
                button_y = sidebar_y;  // start at the very top
                button_h = (current_y - sidebar_y) + large_icon_size + bottom_padding + extra_bottom_padding;
            } else {
                // Other icons: normal padding around the icon
                button_y = current_y - top_padding;
                if (button_y < sidebar_y) {
                    button_y = sidebar_y;
                }
                button_h = large_icon_size + top_padding + bottom_padding;
            }

            int button_w = sidebar_width;  // full width of sidebar

            Paint_DrawRectangle(button_x, button_y,
                                button_x + button_w - 1, button_y + button_h - 1,
                                BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);

            // Draw white (inverted) icon on black background at same position as inactive
            Paint_DrawImageInverted(navItems[i].icon, current_icon_x, current_y, current_icon_size, current_icon_size);
        } else {
            // Draw black icon on white background
            Paint_DrawImage(navItems[i].icon, current_icon_x, current_y, current_icon_size, current_icon_size);
        }
        
        // Move to next icon position using equal spacing for all page icons.
        current_y += button_height + page_icon_gap;
    }
    
    // Bottom: up / down (10x10) / switch (15x15) icons stack
    const int bottom_gap = 4;
    const int bottom_button_padding = 8; // padding from bottom border
    const int button_icon_padding = 4; // padding around up/down icons
    int switch_y = sidebar_y + sidebar_height - bottom_button_padding - large_icon_size; // switch is 15x15
    
    // Calculate button areas with padding
    int down_button_bottom = switch_y - bottom_gap; // bottom of down button area
    int down_button_top = down_button_bottom - icon_size - (button_icon_padding * 2); // top of down button area
    int down_y = down_button_top + button_icon_padding; // icon position within down button area
    
    int up_button_bottom = down_button_top - bottom_gap; // bottom of up button area
    int up_button_top = up_button_bottom - icon_size - (button_icon_padding * 2); // top of up button area
    int up_y = up_button_top + button_icon_padding; // icon position within up button area

    // Draw icons
    Paint_DrawImage(button_up_10x10,   small_icon_x, up_y,   icon_size, icon_size);
    // Top border line above the up button
    Paint_DrawLine(sidebar_x + 1, up_button_top, sidebar_x + sidebar_width - 2, up_button_top,
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    // Bottom border line below the up button
    Paint_DrawLine(sidebar_x + 1, up_button_bottom, sidebar_x + sidebar_width - 2, up_button_bottom,
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    Paint_DrawImage(button_down_10x10, small_icon_x, down_y, icon_size, icon_size);
    // Bottom border line below the down button
    Paint_DrawLine(sidebar_x + 1, down_button_bottom, sidebar_x + sidebar_width - 2, down_button_bottom,
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    Paint_DrawImage(navItems[5].icon,  large_icon_x, switch_y, large_icon_size, large_icon_size); 
}

#endif
