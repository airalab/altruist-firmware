#ifdef ALTRUIST_INSIGHT

#include "desk_screen.h"
#include "display_common.h"
#include "../driver/EPD.h"
#include "../paint_driver/GUI_Paint.h"
#include "../paint_driver/fonts/fonts.h"
#include "../utils.h"
#include "../../intl.h"
#include "../../defines.h"
#include "../../config_manager/config_helpers.h"
#include "../icons/icons/34x34/dust_34x34.h"
#include "../icons/icons/34x32/wi_thermometer_cropped_34x32.h"
#include "../icons/icons/32x32/co2_svgrepo_com_32x32.h"
#include "../icons/icons/34x34/ear_hearing_34x34.h"
#include "../icons/icons/32x32/pressure_32x32.h"
#include "../icons/icons/16x16/warning_new_16x16.h"
#include <stdio.h>
#include <string.h>

static bool deskNoDataGeneric(float value) {
	return value > -1.1f && value < -0.9f;
}

static bool deskNoDataTemp(float value) {
	return value < -200.0f;
}

static void deskFormat(char *out, size_t n, float value, uint8_t precision, bool is_temp) {
	if (!out || n == 0) {
		return;
	}
	if (is_temp ? deskNoDataTemp(value) : deskNoDataGeneric(value)) {
		snprintf(out, n, "%s", INTL_DISP_NO_DATA);
		return;
	}
	stringFromFloat(out, value, precision);
}

uint8_t deskEffectivePagesMask() {
	unsigned mask = cfg::desk_pages & DESK_PAGES_ALL;
	if (cfg::standalone) {
		mask &= ~((1u << DESK_PAGE_AIR) | (1u << DESK_PAGE_NOISE));
	}
	if (mask == 0) {
		mask = (1u << DESK_PAGE_CO2) | (1u << DESK_PAGE_CLIMATE) | (1u << DESK_PAGE_PRESSURE);
		if (!cfg::standalone) {
			mask = DESK_PAGES_ALL;
		}
	}
	return static_cast<uint8_t>(mask);
}

uint8_t deskEnabledCount() {
	uint8_t n = 0;
	const uint8_t mask = deskEffectivePagesMask();
	for (uint8_t i = 0; i < DESK_PAGE_COUNT; i++) {
		if (mask & (1u << i)) {
			n++;
		}
	}
	return n == 0 ? 1 : n;
}

uint8_t deskPageIdAt(uint8_t slot) {
	const uint8_t mask = deskEffectivePagesMask();
	uint8_t seen = 0;
	for (uint8_t i = 0; i < DESK_PAGE_COUNT; i++) {
		if ((mask & (1u << i)) == 0) {
			continue;
		}
		if (seen == slot) {
			return i;
		}
		seen++;
	}
	return DESK_PAGE_CLIMATE;
}

uint8_t deskHomeSlot() {
	const uint8_t mask = deskEffectivePagesMask();
	const uint8_t want = static_cast<uint8_t>(cfg::desk_home);
	uint8_t slot = 0;
	uint8_t first = 0;
	bool have_first = false;
	for (uint8_t i = 0; i < DESK_PAGE_COUNT; i++) {
		if ((mask & (1u << i)) == 0) {
			continue;
		}
		if (!have_first) {
			first = slot;
			have_first = true;
		}
		if (i == want) {
			return slot;
		}
		slot++;
	}
	return first;
}

unsigned deskIntervalMs() {
	unsigned s = cfg::desk_interval_s;
	if (s < DESK_INTERVAL_MIN_S) {
		s = DESK_INTERVAL_MIN_S;
	} else if (s > DESK_INTERVAL_MAX_S) {
		s = DESK_INTERVAL_MAX_S;
	}
	return s * 1000UL;
}

void deskNormalizeConfig() {
	if (cfg::main_layout > MAIN_LAYOUT_DESK) {
		cfg::main_layout = MAIN_LAYOUT_DEFAULT;
	}
	if (cfg::desk_interval_s < DESK_INTERVAL_MIN_S) {
		cfg::desk_interval_s = DESK_INTERVAL_MIN_S;
	} else if (cfg::desk_interval_s > DESK_INTERVAL_MAX_S) {
		cfg::desk_interval_s = DESK_INTERVAL_MAX_S;
	}
	cfg::desk_pages &= DESK_PAGES_ALL;
	if (cfg::desk_pages == 0) {
		cfg::desk_pages = DESK_PAGES_ALL;
	}
	if (cfg::desk_home >= DESK_PAGE_COUNT) {
		cfg::desk_home = DESK_PAGE_AIR;
	}
	if ((cfg::desk_pages & (1u << cfg::desk_home)) == 0) {
		for (uint8_t i = 0; i < DESK_PAGE_COUNT; i++) {
			if (cfg::desk_pages & (1u << i)) {
				cfg::desk_home = i;
				break;
			}
		}
	}
}

static void deskDrawString(uint16_t x, uint16_t y, const char *s, sFONT *en,
                           const Font *ru, const Font *ascii) {
	Paint_DrawString_Display(x, y, s, en, ru, ascii, WHITE, BLACK);
}

static uint16_t deskWidth(const char *s, sFONT *en, const Font *ru, const Font *ascii) {
	return Paint_GetStringWidth_Display(s, en, ru, ascii);
}

static uint16_t deskCenterX(uint16_t col_x, uint16_t col_w, uint16_t w) {
	return (uint16_t)(col_x + (col_w > w ? (col_w - w) / 2 : 0));
}

static const char *deskNextCp(const char *s, uint32_t *cp) {
	const unsigned char *p = reinterpret_cast<const unsigned char *>(s);
	if (p[0] <= 0x7F) {
		*cp = p[0];
		return s + (p[0] == 0 ? 0 : 1);
	}
	if (p[0] >= 0xC2 && p[0] <= 0xDF && p[1]) {
		*cp = (uint32_t)((p[0] & 0x1F) << 6) | (uint32_t)(p[1] & 0x3F);
		return s + 2;
	}
	if (p[0] >= 0xE0 && p[0] <= 0xEF && p[1] && p[2]) {
		*cp = (uint32_t)((p[0] & 0x0F) << 12) | (uint32_t)((p[1] & 0x3F) << 6) | (uint32_t)(p[2] & 0x3F);
		return s + 3;
	}
	*cp = p[0];
	return s + 1;
}

static const Glyph *deskGlyph(const Font *font, uint32_t cp) {
	if (!font) {
		return nullptr;
	}
	for (uint16_t i = 0; i < font->count; i++) {
		if (font->glyphs[i].codepoint == cp) {
			return &font->glyphs[i];
		}
	}
	return nullptr;
}

static uint16_t deskScaleOf(uint8_t scale) {
	return scale == 0 ? 1 : scale;
}

static uint16_t deskDigitGap(uint8_t scale) {
	if (scale <= 1) {
		return 2;
	}
	if (scale == 2) {
		return 4;
	}
	return 3;
}

static uint16_t deskGlyphAdvance(const Glyph *g, uint32_t cp, const Font *font, uint8_t scale) {
	const uint8_t sc = (uint8_t)deskScaleOf(scale);
	uint16_t ink;
	if (cp == 0x20) {
		ink = (font && font->line_height >= 16) ? (uint16_t)(5 * sc) : (uint16_t)(3 * sc);
		return ink;
	}
	if (g) {
		ink = (uint16_t)(g->width * sc);
	} else {
		ink = font ? (uint16_t)((font->line_height / 2) * sc) : 16;
	}
	return (uint16_t)(ink + deskDigitGap(sc));
}

static uint16_t deskHugeWidth(const char *s, const Font *font, uint8_t scale) {
	uint16_t total = 0;
	uint16_t last_adv = 0;
	uint16_t last_ink = 0;
	const uint8_t sc = (uint8_t)deskScaleOf(scale);
	while (s && *s) {
		uint32_t cp = 0;
		s = deskNextCp(s, &cp);
		if (cp == 0) {
			break;
		}
		const Glyph *g = deskGlyph(font, cp);
		last_adv = deskGlyphAdvance(g, cp, font, sc);
		if (g && cp != 0x20) {
			last_ink = (uint16_t)(g->width * sc);
		} else {
			last_ink = last_adv;
		}
		total = (uint16_t)(total + last_adv);
	}
	if (total >= last_adv && last_adv > last_ink) {
		total = (uint16_t)(total - (uint16_t)(last_adv - last_ink));
	}
	return total;
}

static void deskDrawGlyphScaled(const Glyph *g, uint16_t x, uint16_t y, uint8_t scale) {
	if (!g || !g->bitmap || scale == 0) {
		return;
	}
	const uint16_t bw = g->width;
	const uint16_t bh = g->height;
	const uint16_t bytes_per_row = (uint16_t)((bw + 7) / 8);
	const uint8_t *ptr = g->bitmap;
	const int y0 = (int)y + (int)g->top * (int)scale;
	for (uint16_t row = 0; row < bh; row++) {
		for (uint16_t col = 0; col < bw; col++) {
			const uint8_t bit = (ptr[row * bytes_per_row + col / 8] >> (7 - (col % 8))) & 1;
			if (!bit) {
				continue;
			}
			const int px = (int)x + (int)col * (int)scale;
			const int py = y0 + (int)row * (int)scale;
			for (uint8_t sy = 0; sy < scale; sy++) {
				for (uint8_t sx = 0; sx < scale; sx++) {
					const int dx = px + (int)sx;
					const int dy = py + (int)sy;
					if (dx < 0 || dy < 0 || dx >= (int)Paint.Width || dy >= (int)Paint.Height) {
						continue;
					}
					Paint_SetPixel((UWORD)dx, (UWORD)dy, BLACK);
				}
			}
		}
	}
}

static void deskDrawHugeAt(uint16_t x, uint16_t y, const char *s, const Font *font, uint8_t scale) {
	while (s && *s) {
		uint32_t cp = 0;
		s = deskNextCp(s, &cp);
		const Glyph *g = deskGlyph(font, cp);
		if (g && cp != 0x20) {
			deskDrawGlyphScaled(g, x, y, scale);
		}
		x = (uint16_t)(x + deskGlyphAdvance(g, cp, font, scale));
	}
}

struct DeskHuge {
	const Font *font;
	uint8_t scale;
	uint16_t w;
	uint16_t h;
};

static DeskHuge deskHugeOf(const char *s, const Font *font, uint8_t scale) {
	DeskHuge out;
	out.font = font ? font : &font_48_ascii;
	out.scale = scale == 0 ? 1 : scale;
	out.w = deskHugeWidth(s, out.font, out.scale);
	out.h = (uint16_t)(out.font->line_height * out.scale);
	return out;
}

static uint16_t deskFitHeight(const Font *font, uint8_t scale) {
	if (!font || scale == 0) {
		return 0;
	}
	const uint16_t body = (font->line_height > 12) ? (uint16_t)(font->line_height - 12) : font->line_height;
	return (uint16_t)(body * scale);
}

static bool deskHugeFits(const char *s, const Font *font, uint8_t scale, uint16_t max_w, uint16_t max_h) {
	if (!s || !font) {
		return false;
	}
	const uint16_t h = deskFitHeight(font, scale);
	if (max_h > 0 && h > max_h) {
		return false;
	}
	const uint16_t w = deskHugeWidth(s, font, scale);
	return !(max_w > 0 && w > max_w);
}

static DeskHuge deskFitHugePair(const char *a, const char *b, uint16_t max_w, uint16_t max_h,
                               uint8_t max_scale) {
	static const struct {
		const Font *font;
		uint8_t scale;
	} cand[] = {
		{ &font_48_ascii, 3 },
		{ &font_40_ascii, 3 },
		{ &font_48_ascii, 2 },
		{ &font_40_ascii, 2 },
		{ &font_36_ascii, 2 },
		{ &font_32_ascii, 2 },
		{ &font_48_ascii, 1 },
		{ &font_40_ascii, 1 },
		{ &font_36_ascii, 1 },
		{ &font_32_ascii, 1 },
	};
	if (max_scale < 1) {
		max_scale = 1;
	}
	const char *left = (a && a[0] != '\0') ? a : "0";
	for (uint8_t i = 0; i < sizeof(cand) / sizeof(cand[0]); i++) {
		if (cand[i].scale > max_scale) {
			continue;
		}
		if (!deskHugeFits(left, cand[i].font, cand[i].scale, max_w, max_h)) {
			continue;
		}
		if (b && b[0] != '\0' && !deskHugeFits(b, cand[i].font, cand[i].scale, max_w, max_h)) {
			continue;
		}
		return deskHugeOf(left, cand[i].font, cand[i].scale);
	}
	return deskHugeOf(left, &font_48_ascii, 1);
}

static DeskHuge deskFitHuge(const char *s, uint16_t max_w, uint16_t max_h, uint8_t max_scale) {
	return deskFitHugePair(s, nullptr, max_w, max_h, max_scale);
}

static uint16_t deskHugeInkTop(const char *s, const Font *font, uint8_t scale) {
	uint16_t top = 0xFFFF;
	while (s && *s) {
		uint32_t cp = 0;
		s = deskNextCp(s, &cp);
		const Glyph *g = deskGlyph(font, cp);
		if (g && cp != 0x20 && g->top < top) {
			top = g->top;
		}
	}
	if (top == 0xFFFF) {
		top = 0;
	}
	return (uint16_t)(top * scale);
}

static uint16_t deskHugeInkHeight(const char *s, const Font *font, uint8_t scale) {
	uint16_t top = 0xFFFF;
	uint16_t bot = 0;
	while (s && *s) {
		uint32_t cp = 0;
		s = deskNextCp(s, &cp);
		const Glyph *g = deskGlyph(font, cp);
		if (!g || cp == 0x20) {
			continue;
		}
		if (g->top < top) {
			top = g->top;
		}
		const uint16_t gbot = (uint16_t)(g->top + g->height);
		if (gbot > bot) {
			bot = gbot;
		}
	}
	if (top == 0xFFFF || bot <= top) {
		return (uint16_t)((font ? font->line_height : 48) * scale);
	}
	return (uint16_t)((bot - top) * scale);
}

static uint16_t deskDrawTitle(uint16_t x, uint16_t y, uint16_t max_x, const unsigned char *icon,
                              uint16_t iw, uint16_t ih, const char *title) {
	Paint_DrawImage(icon, x, y, iw, ih);
	sFONT *en = &Font20;
	const Font *ru = &font_20_cyrillic;
	const Font *ascii = &font_20_ascii;
	const uint16_t tx = (uint16_t)(x + iw + 6);
	uint16_t avail = (max_x > tx) ? (uint16_t)(max_x - tx) : 0;
	uint16_t tw = deskWidth(title, en, ru, ascii);
	if (avail > 0 && tw > avail) {
		en = &Font16;
		ru = &font_16_cyrillic;
		ascii = &font_16_ascii;
		tw = deskWidth(title, en, ru, ascii);
	}
	if (avail > 0 && tw > avail) {
		en = &Font12;
		ru = &font_12_cyrillic;
		ascii = &font_12_ascii;
		tw = deskWidth(title, en, ru, ascii);
	}
	const uint16_t th = en->Height;
	const uint16_t ty = (uint16_t)(y + (ih > th ? (ih - th) / 2 : 0));
	deskDrawString(tx, ty, title, en, ru, ascii);
	const uint16_t bottom_icon = (uint16_t)(y + ih);
	const uint16_t bottom_text = (uint16_t)(ty + th);
	return (bottom_icon > bottom_text) ? bottom_icon : bottom_text;
}

static void deskDrawFooter(uint16_t content_left, uint16_t content_right,
                           uint8_t pos_1based, uint8_t total, const char *name) {
	char pager[12];
	snprintf(pager, sizeof(pager), "%02u / %02u", (unsigned)pos_1based, (unsigned)total);
	const uint16_t y = (DISPLAY_HEIGHT > 18) ? (uint16_t)(DISPLAY_HEIGHT - 16) : 0;
	deskDrawString((uint16_t)(content_left + 6), y, pager, &Font12, &font_12_cyrillic, &font_12_ascii);
	const uint16_t nw = deskWidth(name, &Font12, &font_12_cyrillic, &font_12_ascii);
	uint16_t nx = content_right > nw + 6 ? (uint16_t)(content_right - nw - 4) : content_left;
	deskDrawString(nx, y, name, &Font12, &font_12_cyrillic, &font_12_ascii);
}

static void deskVLine(uint16_t x, uint16_t y0, uint16_t y1) {
	if (y1 <= y0) {
		return;
	}
	Paint_DrawLine(x, y0, x, y1, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
}

static void deskHLine(uint16_t x0, uint16_t x1, uint16_t y) {
	if (x1 <= x0) {
		return;
	}
	Paint_DrawLine(x0, y, x1, y, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
}

static void deskSplitV(uint16_t x, uint16_t y0, uint16_t y1) {
	const uint16_t pad = 10;
	if (y1 > y0 + pad * 2 + 8) {
		deskVLine(x, (uint16_t)(y0 + pad), (uint16_t)(y1 - pad));
	} else {
		deskVLine(x, y0, y1);
	}
}

static void deskSplitH(uint16_t x0, uint16_t x1, uint16_t y) {
	const uint16_t pad = 16;
	if (x1 > x0 + pad * 2 + 8) {
		deskHLine((uint16_t)(x0 + pad), (uint16_t)(x1 - pad), y);
	} else {
		deskHLine(x0, x1, y);
	}
}

static void deskDrawCentered(uint16_t col_x, uint16_t col_w, uint16_t y, const char *s,
                             sFONT *en, const Font *ru, const Font *ascii) {
	if (!s || s[0] == '\0') {
		return;
	}
	const uint16_t w = deskWidth(s, en, ru, ascii);
	deskDrawString(deskCenterX(col_x, col_w, w), y, s, en, ru, ascii);
}

static void deskDrawCenteredWarn(uint16_t col_x, uint16_t col_w, uint16_t y,
                                 const char *s, bool warn) {
	if (!s || s[0] == '\0') {
		return;
	}
	const uint16_t tw = deskWidth(s, &Font12, &font_12_cyrillic, &font_12_ascii);
	const uint16_t icon = 16;
	const uint16_t gap = 4;
	const uint16_t group = warn ? (uint16_t)(tw + gap + icon) : tw;
	const uint16_t x = deskCenterX(col_x, col_w, group);
	deskDrawString(x, y, s, &Font12, &font_12_cyrillic, &font_12_ascii);
	if (warn) {
		const uint16_t iy = (Font12.Height > icon)
		                        ? y
		                        : (uint16_t)(y + (Font12.Height - icon) / 2);
		Paint_DrawImage(warning_new_16x16, (uint16_t)(x + tw + gap), iy, icon, icon);
	}
}

static bool deskHasText(const char *s) {
	return s && s[0] != '\0';
}

static uint16_t deskPairInkH(const char *a, const char *b, const Font *font, uint8_t scale) {
	const uint16_t ha = deskHugeInkHeight(a, font, scale);
	const uint16_t hb = (b && b[0] != '\0') ? deskHugeInkHeight(b, font, scale) : ha;
	return ha > hb ? ha : hb;
}

static uint16_t deskSideWAt(const char *s, sFONT *en, const Font *ru, const Font *ascii) {
	if (!deskHasText(s)) {
		return 0;
	}
	return deskWidth(s, en, ru, ascii);
}

static uint16_t deskSideW(const char *s) {
	return deskSideWAt(s, &Font16, &font_16_cyrillic, &font_16_ascii);
}

static void deskDrawHugeWithSide(uint16_t col_x, uint16_t col_w, uint16_t ink_y,
                                 const char *value, const char *side,
                                 const Font *font, uint8_t scale, bool suffix_top,
                                 sFONT *side_en, const Font *side_ru, const Font *side_ascii) {
	if (!deskHasText(value)) {
		return;
	}
	if (!side_en) {
		side_en = &Font16;
		side_ru = &font_16_cyrillic;
		side_ascii = &font_16_ascii;
	}
	const DeskHuge local = deskHugeOf(value, font, scale);
	const uint16_t side_w = deskSideWAt(side, side_en, side_ru, side_ascii);
	const uint16_t total = (uint16_t)(local.w + (side_w ? (uint16_t)(4 + side_w) : 0));
	const uint16_t x = deskCenterX(col_x, col_w, total);
	const uint16_t ink_top = deskHugeInkTop(value, local.font, local.scale);
	const int dy = (int)ink_y - (int)ink_top;
	deskDrawHugeAt(x, dy < 0 ? 0 : (uint16_t)dy, value, local.font, local.scale);
	if (side_w) {
		const uint16_t ink_h = deskHugeInkHeight(value, local.font, local.scale);
		uint16_t sy = ink_y;
		if (!suffix_top && ink_h > side_en->Height) {
			sy = (uint16_t)(ink_y + (ink_h - side_en->Height) / 2);
		}
		deskDrawString((uint16_t)(x + local.w + 4), sy, side, side_en, side_ru, side_ascii);
	}
}

static void deskDrawHugeWithSide(uint16_t col_x, uint16_t col_w, uint16_t ink_y,
                                 const char *value, const char *side,
                                 const Font *font, uint8_t scale, bool suffix_top) {
	deskDrawHugeWithSide(col_x, col_w, ink_y, value, side, font, scale, suffix_top,
	                     &Font16, &font_16_cyrillic, &font_16_ascii);
}

static const char *deskBandPm(float pm25, float pm10) {
	const bool have25 = !deskNoDataGeneric(pm25);
	const bool have10 = !deskNoDataGeneric(pm10);
	if (!have25 && !have10) {
		return nullptr;
	}
	if ((have25 && pm25 >= 55.f) || (have10 && pm10 >= 100.f)) {
		return INTL_DESK_BAND_HIGH;
	}
	if ((have25 && pm25 >= 15.f) || (have10 && pm10 >= 45.f)) {
		return INTL_DESK_BAND_ELEVATED;
	}
	return INTL_DESK_BAND_GOOD;
}

static const char *deskBandNoise(float avg, float mx) {
	const float v = deskNoDataGeneric(mx) ? avg : mx;
	if (deskNoDataGeneric(v)) {
		return nullptr;
	}
	if (v >= 70.f) {
		return INTL_DESK_BAND_HIGH;
	}
	if (v >= 50.f) {
		return INTL_DESK_BAND_ELEVATED;
	}
	return INTL_DESK_BAND_GOOD;
}

static const char *deskBandPress(float outdoor, float indoor) {
	const float v = deskNoDataGeneric(indoor) ? outdoor : indoor;
	if (deskNoDataGeneric(v)) {
		return nullptr;
	}
	if (v < 740.f) {
		return INTL_DESK_BAND_LOW;
	}
	if (v >= 780.f) {
		return INTL_DESK_BAND_HIGH;
	}
	return INTL_DESK_BAND_GOOD;
}

static bool deskClimateTempWarn(float t) {
	if (deskNoDataTemp(t) || t < -40.f || t > 80.f) {
		return false;
	}
	return t < 10.f || t >= 27.f;
}

static bool deskClimateHumWarn(float h) {
	if (deskNoDataGeneric(h) || h < 0.f || h > 120.f) {
		return false;
	}
	return h < 40.f || h >= 70.f;
}

static const char *deskBandCo2(float ppm) {
	if (deskNoDataGeneric(ppm)) {
		return nullptr;
	}
	if (ppm < 1000.f) {
		return INTL_DESK_BAND_GOOD;
	}
	if (ppm < 2000.f) {
		return INTL_DESK_BAND_ELEVATED;
	}
	return INTL_DESK_BAND_HIGH;
}

static bool deskBandWarn(const char *band) {
	return deskHasText(band) && (strcmp(band, INTL_DESK_BAND_HIGH) == 0 ||
	                             strcmp(band, INTL_DESK_BAND_LOW) == 0);
}

static uint16_t deskStatusH(const char *band) {
	if (!deskHasText(band)) {
		return 0;
	}
	return (uint16_t)(Font24.Height + 10);
}

static void deskDrawStatus(uint16_t x0, uint16_t x1, uint16_t bot, const char *band) {
	if (!deskHasText(band) || x1 <= x0) {
		return;
	}
	const uint16_t h = deskStatusH(band);
	const uint16_t y0 = (bot > h) ? (uint16_t)(bot - h) : 0;
	const uint16_t tw = deskWidth(band, &Font24, &font_24_cyrillic, &font_24_ascii);
	const bool warn = deskBandWarn(band);
	const uint16_t icon = 16;
	const uint16_t gap = 6;
	const uint16_t group_w = warn ? (uint16_t)(tw + gap + icon) : tw;
	const uint16_t gx = deskCenterX(x0, (uint16_t)(x1 - x0), group_w);
	const uint16_t ty = (uint16_t)(y0 + (h > Font24.Height ? (h - Font24.Height) / 2 : 0));
	deskHLine((uint16_t)(x0 + 24), (uint16_t)(x1 - 24), y0);
	deskDrawString(gx, ty, band, &Font24, &font_24_cyrillic, &font_24_ascii);
	if (warn) {
		const uint16_t iy = (uint16_t)(ty + (Font24.Height > icon ? (Font24.Height - icon) / 2 : 0));
		Paint_DrawImage(warning_new_16x16, (uint16_t)(gx + tw + gap), iy, icon, icon);
	}
}

static uint16_t deskRowChrome(bool kick, bool cap, bool unit) {
	uint16_t h = 0;
	if (kick) {
		h = (uint16_t)(h + Font12.Height + 4);
	}
	if (cap) {
		h = (uint16_t)(h + Font16.Height + 8);
	}
	if (unit) {
		h = (uint16_t)(h + 6 + Font16.Height);
	}
	return h;
}

static void deskDrawTwoUp(uint16_t left_x, uint16_t right_x, uint16_t col_w, uint16_t mid,
                          uint16_t stage_top, uint16_t stage_bot,
                          const char *kL, const char *cL, const char *vL, const char *uL,
                          const char *kR, const char *cR, const char *vR, const char *uR,
                          const char *band, bool solo) {
	const uint16_t span_w = (uint16_t)((right_x + col_w) - left_x);
	const uint16_t x1 = solo ? left_x : right_x;
	const uint16_t w = solo ? span_w : col_w;
	const bool kick = solo ? deskHasText(kR) : (deskHasText(kL) || deskHasText(kR));
	const bool cap = solo ? deskHasText(cR) : (deskHasText(cL) || deskHasText(cR));
	const bool unit = solo ? deskHasText(uR) : (deskHasText(uL) || deskHasText(uR));
	const uint16_t chrome = deskRowChrome(kick, cap, unit);
	const uint16_t status_h = deskStatusH(band);
	const uint16_t num_bot = (stage_bot > status_h) ? (uint16_t)(stage_bot - status_h) : stage_bot;
	const uint16_t stage_h = (num_bot > stage_top) ? (uint16_t)(num_bot - stage_top) : 80;
	uint16_t max_num_h = 72;
	if (stage_h > chrome + 8) {
		const uint16_t room = (uint16_t)(stage_h - chrome);
		if (room < max_num_h) {
			max_num_h = room;
		}
	}
	const DeskHuge m = solo ? deskFitHuge(vR, w, max_num_h, 2)
	                        : deskFitHugePair(vL, vR, col_w, max_num_h, 2);
	const uint16_t ink_h = solo ? deskHugeInkHeight(vR, m.font, m.scale)
	                            : deskPairInkH(vL, vR, m.font, m.scale);
	const uint16_t card_h = (uint16_t)(chrome + ink_h);
	uint16_t extra = (stage_h > card_h) ? (uint16_t)(stage_h - card_h) : 0;
	uint16_t y = (uint16_t)(stage_top + extra / 2);

	if (kick) {
		if (!solo) {
			deskDrawCentered(left_x, w, y, kL, &Font12, &font_12_cyrillic, &font_12_ascii);
		}
		deskDrawCentered(x1, w, y, kR, &Font12, &font_12_cyrillic, &font_12_ascii);
		y = (uint16_t)(y + Font12.Height + 4);
	}
	if (cap) {
		if (!solo) {
			deskDrawCentered(left_x, w, y, cL, &Font16, &font_16_cyrillic, &font_16_ascii);
		}
		deskDrawCentered(x1, w, y, cR, &Font16, &font_16_cyrillic, &font_16_ascii);
		y = (uint16_t)(y + Font16.Height + 8);
	}
	if (!solo) {
		deskDrawHugeWithSide(left_x, w, y, vL, "", m.font, m.scale, true);
	}
	deskDrawHugeWithSide(x1, w, y, vR, "", m.font, m.scale, true);
	uint16_t band_bot = (uint16_t)(y + ink_h);
	if (unit) {
		const uint16_t uy = (uint16_t)(y + ink_h + 6);
		if (!solo) {
			deskDrawCentered(left_x, w, uy, uL, &Font16, &font_16_cyrillic, &font_16_ascii);
		}
		deskDrawCentered(x1, w, uy, uR, &Font16, &font_16_cyrillic, &font_16_ascii);
		band_bot = (uint16_t)(uy + Font16.Height);
	}
	uint16_t split_bot = band_bot;
	if (status_h && num_bot > band_bot + 4) {
		split_bot = (uint16_t)(num_bot - 4);
	}
	if (!solo) {
		deskSplitV(mid, stage_top, split_bot);
	}
	deskDrawStatus(left_x, (uint16_t)(left_x + span_w), stage_bot, band);
}

static void deskDrawClimate(uint16_t left_x, uint16_t right_x, uint16_t col_w, uint16_t mid,
                            uint16_t stage_top, uint16_t stage_bot,
                            const char *temp_l, const char *temp_r,
                            const char *hum_l, const char *hum_r, bool solo,
                            bool warn_tL, bool warn_tR, bool warn_hL, bool warn_hR) {
	const uint16_t span_w = (uint16_t)((right_x + col_w) - left_x);
	const uint16_t x1 = solo ? left_x : right_x;
	const uint16_t w = solo ? span_w : col_w;
	const char *tu = cfgTempDisplayUnit();
	const uint16_t tu_w = deskSideWAt(tu, &Font24, &font_24_cyrillic, &font_24_ascii);
	const uint16_t suffix_w = tu_w > deskSideW("%") ? tu_w : deskSideW("%");
	uint16_t max_num_w = w;
	if (suffix_w && w > suffix_w + 6) {
		max_num_w = (uint16_t)(w - suffix_w - 6);
	}
	const uint16_t kicker_h = (uint16_t)(Font12.Height + 4);
	const uint16_t label_gap = 10;
	const uint16_t label_h = (uint16_t)(label_gap + Font12.Height);
	const uint16_t fit_label_h = (uint16_t)(4 + Font12.Height);
	const uint16_t stage_h = (stage_bot > stage_top) ? (uint16_t)(stage_bot - stage_top) : 80;
	const uint16_t split_y = (uint16_t)(stage_top + stage_h / 2);
	const uint16_t upper_h = (split_y > stage_top) ? (uint16_t)(split_y - stage_top) : 80;
	const uint16_t lower_h = (stage_bot > split_y) ? (uint16_t)(stage_bot - split_y) : 48;
	uint16_t temp_max_h = 72;
	if (upper_h > kicker_h + fit_label_h + 8) {
		temp_max_h = (uint16_t)(upper_h - kicker_h - fit_label_h);
		if (temp_max_h > 72) {
			temp_max_h = 72;
		}
	}
	uint16_t hum_max_h = 72;
	if (lower_h > fit_label_h + 8) {
		hum_max_h = (uint16_t)(lower_h - fit_label_h);
		if (hum_max_h > 72) {
			hum_max_h = 72;
		}
	}
	if (hum_max_h > temp_max_h) {
		hum_max_h = temp_max_h;
	}
	const DeskHuge t = solo ? deskFitHuge(temp_r, max_num_w, temp_max_h, 2)
	                        : deskFitHugePair(temp_l, temp_r, max_num_w, temp_max_h, 2);
	const uint16_t t_ink = solo ? deskHugeInkHeight(temp_r, t.font, t.scale)
	                            : deskPairInkH(temp_l, temp_r, t.font, t.scale);
	const uint16_t upper_card = (uint16_t)(kicker_h + t_ink + label_h);
	const uint16_t line_y = (uint16_t)(split_y + 10);
	uint16_t y = stage_top;
	if (upper_h > upper_card) {
		y = (uint16_t)(stage_top + (upper_h - upper_card) / 2);
	}
	if (!solo) {
		deskDrawCentered(left_x, w, y, "URBAN", &Font12, &font_12_cyrillic, &font_12_ascii);
	}
	deskDrawCentered(x1, w, y, "INSIGHT", &Font12, &font_12_cyrillic, &font_12_ascii);
	y = (uint16_t)(y + kicker_h);
	if (!solo) {
		deskDrawHugeWithSide(left_x, w, y, temp_l, tu, t.font, t.scale, false,
		                     &Font24, &font_24_cyrillic, &font_24_ascii);
	}
	deskDrawHugeWithSide(x1, w, y, temp_r, tu, t.font, t.scale, false,
	                     &Font24, &font_24_cyrillic, &font_24_ascii);
	y = (uint16_t)(y + t_ink + label_gap);
	if (!solo) {
		deskDrawCenteredWarn(left_x, w, y, INTL_DISP_TEMPERATURE, warn_tL);
	}
	deskDrawCenteredWarn(x1, w, y, INTL_DISP_TEMPERATURE, warn_tR);

	if (!solo && t_ink > 8 && hum_max_h > (uint16_t)(t_ink - 8)) {
		hum_max_h = (uint16_t)(t_ink - 8);
	}
	const DeskHuge h = solo ? deskFitHuge(hum_r, max_num_w, hum_max_h, 2)
	                        : deskFitHugePair(hum_l, hum_r, max_num_w, hum_max_h, 2);
	const uint16_t h_ink = solo ? deskHugeInkHeight(hum_r, h.font, h.scale)
	                            : deskPairInkH(hum_l, hum_r, h.font, h.scale);
	const uint16_t lower_card = (uint16_t)(h_ink + label_h);
	const uint16_t hum_pad_top = 14;
	const uint16_t vis_top = (uint16_t)(line_y + hum_pad_top);
	const uint16_t vis_lower = (stage_bot > vis_top) ? (uint16_t)(stage_bot - vis_top) : 0;
	uint16_t hum_y = vis_top;
	if (vis_lower > lower_card) {
		hum_y = (uint16_t)(vis_top + (vis_lower - lower_card) / 2);
	}
	if (!solo) {
		deskDrawHugeWithSide(left_x, w, hum_y, hum_l, "%", h.font, h.scale, false);
	}
	deskDrawHugeWithSide(x1, w, hum_y, hum_r, "%", h.font, h.scale, false);
	const uint16_t hum_label_y = (uint16_t)(hum_y + h_ink + label_gap);
	if (!solo) {
		deskDrawCenteredWarn(left_x, w, hum_label_y, INTL_DISP_HUMIDITY, warn_hL);
	}
	deskDrawCenteredWarn(x1, w, hum_label_y, INTL_DISP_HUMIDITY, warn_hR);
	if (!solo) {
		deskSplitV(mid, stage_top, stage_bot);
	}
	deskSplitH(left_x, (uint16_t)(left_x + span_w), line_y);
}

static int deskCo2Mood(const char *band) {
	if (deskHasText(band) && strcmp(band, INTL_DESK_BAND_HIGH) == 0) {
		return 2;
	}
	if (deskHasText(band) && strcmp(band, INTL_DESK_BAND_ELEVATED) == 0) {
		return 1;
	}
	return 0;
}

static void deskDrawFace(uint16_t cx, uint16_t cy, uint16_t r, int mood) {
	if (r < 12) {
		return;
	}
	Paint_DrawCircle(cx, cy, r, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
	if (r > 2) {
		Paint_DrawCircle(cx, cy, (uint16_t)(r - 1), BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
	}
	uint16_t eye_r = (uint16_t)(r / 7);
	if (eye_r < 3) {
		eye_r = 3;
	}
	const uint16_t eye_dx = (uint16_t)((r * 5) / 16);
	const uint16_t eye_y = (uint16_t)(cy - r / 5);
	Paint_DrawCircle((uint16_t)(cx - eye_dx), eye_y, eye_r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
	Paint_DrawCircle((uint16_t)(cx + eye_dx), eye_y, eye_r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);

	const uint16_t mw = (uint16_t)((r * 10) / 16);
	const uint16_t mouth_y = (uint16_t)(cy + r / 5);
	const uint16_t bow = (uint16_t)(r / 5);
	if (mood == 1 || mw < 4) {
		Paint_DrawLine((uint16_t)(cx - mw), mouth_y, (uint16_t)(cx + mw), mouth_y,
		               BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
		return;
	}
	int prev_x = -1;
	int prev_y = -1;
	for (int x = -(int)mw; x <= (int)mw; x++) {
		const int t2 = (x * x * (int)bow) / ((int)mw * (int)mw);
		const int yy = (mood == 0) ? ((int)mouth_y + (int)bow - t2) : ((int)mouth_y + t2);
		const int xx = (int)cx + x;
		if (prev_x >= 0) {
			Paint_DrawLine((UWORD)prev_x, (UWORD)prev_y, (UWORD)xx, (UWORD)yy,
			               BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
		}
		prev_x = xx;
		prev_y = yy;
	}
}

static void deskDrawCo2(uint16_t content_left, uint16_t area_w,
                        uint16_t stage_top, uint16_t stage_bot,
                        const char *value, const char *band) {
	const char *ppm = "ppm";
	const uint16_t ppm_w = deskWidth(ppm, &Font16, &font_16_cyrillic, &font_16_ascii);
	const uint16_t kicker_h = (uint16_t)(Font12.Height + 8);
	const bool have_face = deskHasText(band);
	uint16_t face_r = 28;
	const uint16_t word_h = (uint16_t)(Font24.Height + 4);
	const uint16_t mid_gap = 16;
	uint16_t face_block = 0;
	if (have_face) {
		uint16_t d = (uint16_t)(face_r * 2);
		face_block = (uint16_t)(mid_gap + d + 8 + word_h);
	}
	const uint16_t hero_bot = (have_face && stage_bot > face_block)
	                              ? (uint16_t)(stage_bot - face_block)
	                              : stage_bot;
	const uint16_t hero_h = (hero_bot > stage_top) ? (uint16_t)(hero_bot - stage_top) : 48;
	uint16_t max_num_h = (hero_h > kicker_h + 8) ? (uint16_t)(hero_h - kicker_h) : 48;
	if (max_num_h > 96) {
		max_num_h = 96;
	}
	uint16_t max_num_w = (area_w > ppm_w + 16) ? (uint16_t)(area_w - ppm_w - 16) : area_w;
	const DeskHuge huge = deskFitHuge(value, max_num_w, max_num_h, 2);
	const uint16_t ink_h = deskHugeInkHeight(value, huge.font, huge.scale);
	const uint16_t group_w = (uint16_t)(huge.w + 8 + ppm_w);
	const uint16_t group_h = (uint16_t)(kicker_h + ink_h);
	uint16_t extra = (hero_h > group_h) ? (uint16_t)(hero_h - group_h) : 0;
	uint16_t y = (uint16_t)(stage_top + extra / 8);
	const uint16_t gx = deskCenterX(content_left, area_w, group_w);
	deskDrawCentered(gx, huge.w, y, "INSIGHT", &Font12, &font_12_cyrillic, &font_12_ascii);
	const uint16_t ink_y = (uint16_t)(y + kicker_h);
	const uint16_t ink_top = deskHugeInkTop(value, huge.font, huge.scale);
	const int draw_y = (int)ink_y - (int)ink_top;
	deskDrawHugeAt(gx, draw_y < 0 ? 0 : (uint16_t)draw_y, value, huge.font, huge.scale);
	uint16_t ppm_y = ink_y;
	if (ink_h > Font16.Height) {
		ppm_y = (uint16_t)(ink_y + (ink_h - Font16.Height) / 2);
	}
	deskDrawString((uint16_t)(gx + huge.w + 8), ppm_y, ppm, &Font16, &font_16_cyrillic, &font_16_ascii);

	if (!have_face) {
		return;
	}
	const uint16_t split_y = (uint16_t)(hero_bot + mid_gap / 2);
	deskSplitH(content_left, (uint16_t)(content_left + area_w), split_y);
	const uint16_t face_top = (uint16_t)(hero_bot + mid_gap);
	const uint16_t cy = (uint16_t)(face_top + face_r + 3);
	const uint16_t cx = (uint16_t)(content_left + area_w / 2);
	deskDrawFace(cx, cy, face_r, deskCo2Mood(band));
	const uint16_t tw = deskWidth(band, &Font24, &font_24_cyrillic, &font_24_ascii);
	const bool warn = deskBandWarn(band);
	const uint16_t icon = 16;
	const uint16_t gap = 6;
	const uint16_t word_w = warn ? (uint16_t)(tw + gap + icon) : tw;
	const uint16_t wx = deskCenterX(content_left, area_w, word_w);
	const uint16_t wy = (uint16_t)(face_top + face_r * 2 + 8);
	deskDrawString(wx, wy, band, &Font24, &font_24_cyrillic, &font_24_ascii);
	if (warn) {
		const uint16_t iy = (uint16_t)(wy + (Font24.Height > icon ? (Font24.Height - icon) / 2 : 0));
		Paint_DrawImage(warning_new_16x16, (uint16_t)(wx + tw + gap), iy, icon, icon);
	}
}

void drawDeskMainBody(const main_screen_values_t &values,
                      uint16_t body_top, uint16_t content_left, uint16_t content_right,
                      uint8_t page_id, uint8_t pos_1based, uint8_t total) {
	const uint16_t title_y = (uint16_t)(body_top + 2);
	const uint16_t footer_y = (DISPLAY_HEIGHT > 18) ? (uint16_t)(DISPLAY_HEIGHT - 16) : DISPLAY_HEIGHT;
	const unsigned char *icon = dust_34x34;
	uint16_t iw = 34, ih = 34;
	const char *title = INTL_DESK_TITLE_AIR;
	const char *footer = INTL_DESK_FOOTER_AIR;
	switch (page_id) {
	case DESK_PAGE_CLIMATE:
		icon = wi_thermometer_cropped_34x32;
		iw = 34;
		ih = 32;
		title = INTL_DESK_TITLE_CLIMATE;
		footer = INTL_DESK_FOOTER_CLIMATE;
		break;
	case DESK_PAGE_CO2:
		icon = co2_svgrepo_com_32x32;
		iw = 32;
		ih = 32;
		title = INTL_DESK_TITLE_CO2;
		footer = INTL_DESK_FOOTER_CO2;
		break;
	case DESK_PAGE_NOISE:
		icon = ear_hearing_34x34;
		title = INTL_DESK_TITLE_NOISE;
		footer = INTL_DESK_FOOTER_NOISE;
		break;
	case DESK_PAGE_PRESSURE:
		icon = pressure_32x32;
		iw = 32;
		ih = 32;
		title = INTL_DESK_TITLE_PRESSURE;
		footer = INTL_DESK_FOOTER_PRESSURE;
		break;
	default:
		break;
	}
	const uint16_t title_bot = deskDrawTitle(content_left + 6, title_y, content_right, icon, iw, ih, title);
	const uint16_t inner_x0 = (uint16_t)(content_left + 6);
	const uint16_t inner_x1 = (uint16_t)(content_right - 6);
	const uint16_t inner_y0 = (uint16_t)(title_bot + 8);
	const uint16_t inner_y1 = (footer_y > 6) ? (uint16_t)(footer_y - 4) : inner_y0;
	const uint16_t mid = (uint16_t)(inner_x0 + (inner_x1 - inner_x0) / 2);
	const uint16_t left_x = inner_x0;
	const uint16_t right_x = (uint16_t)(mid + 8);
	uint16_t col_w = (mid > left_x + 8) ? (uint16_t)(mid - 8 - left_x) : 80;
	const uint16_t right_w = (inner_x1 > right_x) ? (uint16_t)(inner_x1 - right_x) : col_w;
	if (right_w < col_w) {
		col_w = right_w;
	}
	const uint16_t inner_w = (inner_x1 > inner_x0) ? (uint16_t)(inner_x1 - inner_x0) : 0;

	char a[20], b[20], c[20], d[20];

	switch (page_id) {
	case DESK_PAGE_AIR: {
		const uint8_t p10p = (!deskNoDataGeneric(values.pm10) && values.pm10 > 0.f && values.pm10 < 1.f) ? 1 : 0;
		const uint8_t p25p = (!deskNoDataGeneric(values.pm25) && values.pm25 > 0.f && values.pm25 < 1.f) ? 1 : 0;
		deskFormat(a, sizeof(a), values.pm25, p25p, false);
		deskFormat(b, sizeof(b), values.pm10, p10p, false);
		deskDrawTwoUp(left_x, right_x, col_w, mid, inner_y0, inner_y1,
		              "URBAN", "PM2.5", a, "µg/m³",
		              "URBAN", "PM10", b, "µg/m³",
		              deskBandPm(values.pm25, values.pm10), false);
		break;
	}
	case DESK_PAGE_CLIMATE: {
		deskFormat(a, sizeof(a), tempCToDisplay(values.temp_outdoor), 0, true);
		deskFormat(b, sizeof(b), values.hum_outdoor, 0, false);
		deskFormat(c, sizeof(c), tempCToDisplay(values.temp_indoor), 0, true);
		deskFormat(d, sizeof(d), values.hum_indoor, 0, false);
		deskDrawClimate(left_x, right_x, col_w, mid, inner_y0, inner_y1, a, c, b, d,
		                cfg::standalone,
		                deskClimateTempWarn(values.temp_outdoor),
		                deskClimateTempWarn(values.temp_indoor),
		                deskClimateHumWarn(values.hum_outdoor),
		                deskClimateHumWarn(values.hum_indoor));
		break;
	}
	case DESK_PAGE_CO2:
		deskFormat(a, sizeof(a), values.co2, 0, false);
		deskDrawCo2(inner_x0, inner_w, inner_y0, inner_y1, a, deskBandCo2(values.co2));
		break;
	case DESK_PAGE_NOISE:
		deskFormat(a, sizeof(a), values.noise_avg, 0, false);
		deskFormat(b, sizeof(b), values.noise_max, 0, false);
		deskDrawTwoUp(left_x, right_x, col_w, mid, inner_y0, inner_y1,
		              "URBAN", "AVG", a, "dBA",
		              "URBAN", "MAX", b, "dBA",
		              deskBandNoise(values.noise_avg, values.noise_max), false);
		break;
	case DESK_PAGE_PRESSURE:
	default: {
		deskFormat(a, sizeof(a), pressureMmHgToDisplay(values.press_outdoor), 0, false);
		deskFormat(b, sizeof(b), pressureMmHgToDisplay(values.press_indoor), 0, false);
		const char *pu = cfgPressureDisplayUnit();
		deskDrawTwoUp(left_x, right_x, col_w, mid, inner_y0, inner_y1,
		              "URBAN", "", a, pu,
		              "INSIGHT", "", b, pu,
		              deskBandPress(values.press_outdoor, values.press_indoor),
		              cfg::standalone);
		break;
	}
	}
	deskDrawFooter(content_left, content_right, pos_1based, total, footer);
}

#endif
