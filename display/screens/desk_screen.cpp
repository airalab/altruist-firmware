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
#include "../icons/icons/32x32/urban_32x32.h"
#include "../icons/icons/32x32/insight_32x32.h"
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

static const uint8_t kDeskOrder[DESK_PAGE_COUNT] = {
	DESK_PAGE_AIR,
	DESK_PAGE_CLIMATE,
	DESK_PAGE_CLIMATE_INSIGHT,
	DESK_PAGE_CO2,
	DESK_PAGE_NOISE,
	DESK_PAGE_PRESSURE,
};

uint8_t deskPageIdInOrder(uint8_t idx) {
	if (idx >= DESK_PAGE_COUNT) {
		return DESK_PAGE_CLIMATE;
	}
	return kDeskOrder[idx];
}

uint8_t deskEffectivePagesMask() {
	unsigned mask = cfg::desk_pages & DESK_PAGES_ALL;
	if (cfg::standalone) {
		mask &= ~((1u << DESK_PAGE_AIR) | (1u << DESK_PAGE_NOISE) | (1u << DESK_PAGE_CLIMATE));
	}
	if (mask == 0) {
		mask = (1u << DESK_PAGE_CO2) | (1u << DESK_PAGE_CLIMATE_INSIGHT) |
		       (1u << DESK_PAGE_PRESSURE);
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
		if (mask & (1u << kDeskOrder[i])) {
			n++;
		}
	}
	return n == 0 ? 1 : n;
}

uint8_t deskPageIdAt(uint8_t slot) {
	const uint8_t mask = deskEffectivePagesMask();
	uint8_t seen = 0;
	for (uint8_t i = 0; i < DESK_PAGE_COUNT; i++) {
		const uint8_t id = kDeskOrder[i];
		if ((mask & (1u << id)) == 0) {
			continue;
		}
		if (seen == slot) {
			return id;
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
		const uint8_t id = kDeskOrder[i];
		if ((mask & (1u << id)) == 0) {
			continue;
		}
		if (!have_first) {
			first = slot;
			have_first = true;
		}
		if (id == want) {
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
	if (cfg::desk_pages == DESK_PAGES_ALL_V1) {
		cfg::desk_pages = DESK_PAGES_ALL;
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

static constexpr uint16_t kDeskSrcIcon = 32;
static constexpr uint16_t kDeskSrcScale = 2;
static constexpr uint16_t kDeskSrcDraw = kDeskSrcIcon * kDeskSrcScale;
static constexpr uint16_t kDeskSrcMix = 48;
static constexpr uint16_t kDeskSrcPad = 2;

/** 32×32 1-bit icon, nearest-neighbor 2× — only black dots (canvas stays white). */
static void deskDrawSrcIcon2x(const unsigned char *bits, uint16_t x, uint16_t y) {
	if (!bits) {
		return;
	}
	const uint16_t bpr = (uint16_t)((kDeskSrcIcon + 7) / 8);
	for (uint16_t row = 0; row < kDeskSrcIcon; row++) {
		for (uint16_t col = 0; col < kDeskSrcIcon; col++) {
			const uint8_t byte = bits[row * bpr + col / 8];
			const uint8_t on = (uint8_t)((byte >> (7 - (col % 8))) & 1);
			if (on) {
				continue;
			}
			const uint16_t px = (uint16_t)(x + col * kDeskSrcScale);
			const uint16_t py = (uint16_t)(y + row * kDeskSrcScale);
			Paint_SetPixel(px, py, BLACK);
			Paint_SetPixel((uint16_t)(px + 1), py, BLACK);
			Paint_SetPixel(px, (uint16_t)(py + 1), BLACK);
			Paint_SetPixel((uint16_t)(px + 1), (uint16_t)(py + 1), BLACK);
		}
	}
}

/** 32×32 → 48×48 nearest-neighbor, black pixels only. */
static void deskDrawSrcIconMix(const unsigned char *bits, uint16_t x, uint16_t y) {
	if (!bits) {
		return;
	}
	const uint16_t bpr = (uint16_t)((kDeskSrcIcon + 7) / 8);
	for (uint16_t row = 0; row < kDeskSrcMix; row++) {
		for (uint16_t col = 0; col < kDeskSrcMix; col++) {
			const uint16_t sr = (uint16_t)(row * kDeskSrcIcon / kDeskSrcMix);
			const uint16_t sc = (uint16_t)(col * kDeskSrcIcon / kDeskSrcMix);
			const uint8_t byte = bits[sr * bpr + sc / 8];
			const uint8_t on = (uint8_t)((byte >> (7 - (sc % 8))) & 1);
			if (on) {
				continue;
			}
			Paint_SetPixel((uint16_t)(x + col), (uint16_t)(y + row), BLACK);
		}
	}
}

static uint16_t deskSrcDrawH(uint8_t scale) {
	if (scale >= 2) {
		return kDeskSrcDraw;
	}
	if (scale == 1) {
		return kDeskSrcMix;
	}
	return kDeskSrcIcon;
}

static void deskDrawColSrc(uint16_t col_x, uint16_t col_w, uint16_t y,
                           const unsigned char *bits, uint8_t scale) {
	(void)col_w;
	if (!bits) {
		return;
	}
	const uint16_t x = (uint16_t)(col_x + kDeskSrcPad);
	if (scale >= 2) {
		deskDrawSrcIcon2x(bits, x, y);
	} else if (scale == 1) {
		deskDrawSrcIconMix(bits, x, y);
	} else {
		Paint_DrawImage(bits, x, y, kDeskSrcIcon, kDeskSrcIcon);
	}
}

static const unsigned char *deskSrcBits(const char *s) {
	if (!s || s[0] == '\0') {
		return nullptr;
	}
	if (strcmp(s, INTL_DISP_MAIN_URBAN) == 0 || strcmp(s, "URBAN") == 0) {
		return urban_32x32;
	}
	return insight_32x32;
}

static uint16_t deskDrawTitle(uint16_t x, uint16_t y, uint16_t max_x, const unsigned char *icon,
                              uint16_t iw, uint16_t ih, const char *title,
                              const unsigned char *src_icon) {
	uint16_t cur = x;
	if (icon && iw && ih) {
		Paint_DrawImage(icon, cur, y, iw, ih);
		cur = (uint16_t)(cur + iw + 6);
	}
	uint16_t src_x = max_x;
	if (src_icon && max_x > kDeskSrcDraw) {
		src_x = (uint16_t)(max_x - kDeskSrcDraw);
		if (src_x < cur + 8) {
			src_x = (uint16_t)(cur + 8);
		}
		deskDrawSrcIcon2x(src_icon, src_x, y);
	}
	sFONT *en = &Font24;
	const Font *ru = &font_24_cyrillic;
	const Font *ascii = &font_24_ascii;
	uint16_t title_max = (src_icon && src_x > 6) ? (uint16_t)(src_x - 6) : max_x;
	uint16_t avail = (title_max > cur) ? (uint16_t)(title_max - cur) : 0;
	uint16_t tw = deskWidth(title, en, ru, ascii);
	if (avail > 0 && tw > avail) {
		en = &Font20;
		ru = &font_20_cyrillic;
		ascii = &font_20_ascii;
		tw = deskWidth(title, en, ru, ascii);
	}
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
	deskDrawString(cur, ty, title, en, ru, ascii);
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

static bool deskHasText(const char *s) {
	return s && s[0] != '\0';
}

static void deskDrawCentered(uint16_t col_x, uint16_t col_w, uint16_t y, const char *s,
                             sFONT *en, const Font *ru, const Font *ascii) {
	if (!s || s[0] == '\0') {
		return;
	}
	const uint16_t w = deskWidth(s, en, ru, ascii);
	deskDrawString(deskCenterX(col_x, col_w, w), y, s, en, ru, ascii);
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

static const char *deskBandOneTemp(float t) {
	if (deskNoDataTemp(t) || t < -40.f || t > 80.f) {
		return nullptr;
	}
	if (t < 10.f) {
		return INTL_DESK_BAND_LOW;
	}
	if (t >= 27.f) {
		return INTL_DESK_BAND_HIGH;
	}
	return INTL_DESK_BAND_GOOD;
}

static const char *deskBandOneHum(float h) {
	if (deskNoDataGeneric(h) || h < 0.f || h > 120.f) {
		return nullptr;
	}
	if (h < 40.f) {
		return INTL_DESK_BAND_LOW;
	}
	if (h >= 70.f) {
		return INTL_DESK_BAND_HIGH;
	}
	return INTL_DESK_BAND_GOOD;
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

static void deskDrawStatusWord(uint16_t col_x, uint16_t col_w, uint16_t y, const char *band) {
	if (!deskHasText(band) || col_w == 0) {
		return;
	}
	const uint16_t tw = deskWidth(band, &Font24, &font_24_cyrillic, &font_24_ascii);
	const bool warn = deskBandWarn(band);
	const uint16_t icon = 16;
	const uint16_t gap = 6;
	const uint16_t group_w = warn ? (uint16_t)(tw + gap + icon) : tw;
	const uint16_t gx = deskCenterX(col_x, col_w, group_w);
	deskDrawString(gx, y, band, &Font24, &font_24_cyrillic, &font_24_ascii);
	if (warn) {
		const uint16_t iy = (uint16_t)(y + (Font24.Height > icon ? (Font24.Height - icon) / 2 : 0));
		Paint_DrawImage(warning_new_16x16, (uint16_t)(gx + tw + gap), iy, icon, icon);
	}
}

static void deskDrawStatus(uint16_t x0, uint16_t x1, uint16_t bot, const char *band) {
	if (!deskHasText(band) || x1 <= x0) {
		return;
	}
	const uint16_t h = deskStatusH(band);
	const uint16_t y0 = (bot > h) ? (uint16_t)(bot - h) : 0;
	const uint16_t ty = (uint16_t)(y0 + (h > Font24.Height ? (h - Font24.Height) / 2 : 0));
	deskHLine((uint16_t)(x0 + 24), (uint16_t)(x1 - 24), y0);
	deskDrawStatusWord(x0, (uint16_t)(x1 - x0), ty, band);
}

static void deskDrawStatusPair(uint16_t left_x, uint16_t right_x, uint16_t col_w,
                              uint16_t x1, uint16_t bot,
                              const char *band_l, const char *band_r) {
	if ((!deskHasText(band_l) && !deskHasText(band_r)) || col_w == 0) {
		return;
	}
	const uint16_t h = (uint16_t)(Font24.Height + 10);
	const uint16_t y0 = (bot > h) ? (uint16_t)(bot - h) : 0;
	const uint16_t ty = (uint16_t)(y0 + (h > Font24.Height ? (h - Font24.Height) / 2 : 0));
	const uint16_t span_w = (uint16_t)((right_x + col_w) - left_x);
	deskHLine((uint16_t)(left_x + 24), (uint16_t)(left_x + span_w - 24), y0);
	deskDrawStatusWord(left_x, col_w, ty, band_l);
	deskDrawStatusWord(x1, col_w, ty, band_r);
}

static uint16_t deskRowChrome(bool kick, bool cap, bool unit, bool unit_lg) {
	uint16_t h = 0;
	if (kick) {
		h = (uint16_t)(h + deskSrcDrawH(1) + 4);
	}
	if (cap) {
		h = (uint16_t)(h + Font16.Height + 8);
	}
	if (unit) {
		h = (uint16_t)(h + 6 + (unit_lg ? Font24.Height : Font16.Height));
	}
	return h;
}

static void deskDrawTwoUp(uint16_t left_x, uint16_t right_x, uint16_t col_w, uint16_t mid,
                          uint16_t stage_top, uint16_t stage_bot,
                          const char *kL, const char *cL, const char *vL, const char *uL,
                          const char *kR, const char *cR, const char *vR, const char *uR,
                          const char *band, bool solo, bool unit_lg = false,
                          const char *band_l = nullptr, const char *band_r = nullptr) {
	const uint16_t span_w = (uint16_t)((right_x + col_w) - left_x);
	const uint16_t x1 = solo ? left_x : right_x;
	const uint16_t w = solo ? span_w : col_w;
	const bool kick = solo ? deskHasText(kR) : (deskHasText(kL) || deskHasText(kR));
	const bool cap = solo ? deskHasText(cR) : (deskHasText(cL) || deskHasText(cR));
	const bool unit = solo ? deskHasText(uR) : (deskHasText(uL) || deskHasText(uR));
	const bool dual = deskHasText(band_l) || deskHasText(band_r);
	const uint16_t chrome = deskRowChrome(kick, cap, unit, unit_lg);
	const uint16_t status_h = dual ? (uint16_t)(Font24.Height + 10) : deskStatusH(band);
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
		const uint16_t kh = (uint16_t)(deskSrcDrawH(1) + 4);
		const uint16_t icon_y = (uint16_t)(stage_top + 2);
		if (!solo) {
			deskDrawColSrc(left_x, w, icon_y, deskSrcBits(kL), 1);
		}
		deskDrawColSrc(x1, w, icon_y, deskSrcBits(kR), 1);
		y = (uint16_t)(y + kh);
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
		sFONT *uen = unit_lg ? &Font24 : &Font16;
		const Font *uru = unit_lg ? &font_24_cyrillic : &font_16_cyrillic;
		const Font *uascii = unit_lg ? &font_24_ascii : &font_16_ascii;
		const uint16_t uy = (uint16_t)(y + ink_h + 6);
		if (!solo) {
			deskDrawCentered(left_x, w, uy, uL, uen, uru, uascii);
		}
		deskDrawCentered(x1, w, uy, uR, uen, uru, uascii);
		band_bot = (uint16_t)(uy + uen->Height);
	}
	uint16_t split_bot = band_bot;
	if (status_h && num_bot > band_bot + 4) {
		split_bot = (uint16_t)(num_bot - 4);
	}
	if (!solo) {
		deskSplitV(mid, stage_top, split_bot);
	}
	if (dual) {
		deskDrawStatusPair(left_x, right_x, w, x1, stage_bot, band_l, band_r);
	} else {
		deskDrawStatus(left_x, (uint16_t)(left_x + span_w), stage_bot, band);
	}
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
	uint16_t max_num_h = (hero_h > 8) ? (uint16_t)(hero_h - 8) : 48;
	if (max_num_h > 96) {
		max_num_h = 96;
	}
	uint16_t max_num_w = (area_w > ppm_w + 16) ? (uint16_t)(area_w - ppm_w - 16) : area_w;
	const DeskHuge huge = deskFitHuge(value, max_num_w, max_num_h, 2);
	const uint16_t ink_h = deskHugeInkHeight(value, huge.font, huge.scale);
	const uint16_t group_w = (uint16_t)(huge.w + 8 + ppm_w);
	uint16_t extra = (hero_h > ink_h) ? (uint16_t)(hero_h - ink_h) : 0;
	uint16_t drop = (uint16_t)(extra / 3);
	if (drop < 32 && extra >= 32) {
		drop = 32;
	}
	uint16_t y = (uint16_t)(stage_top + drop);
	const uint16_t gx = deskCenterX(content_left, area_w, group_w);
	const uint16_t ink_y = y;
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
	const unsigned char *src_icon = urban_32x32;
	const char *title = INTL_DESK_TITLE_AIR;
	const char *footer = INTL_DESK_FOOTER_AIR;
	switch (page_id) {
	case DESK_PAGE_CLIMATE:
		icon = wi_thermometer_cropped_34x32;
		iw = 34;
		ih = 32;
		src_icon = urban_32x32;
		title = INTL_DESK_TITLE_CLIMATE;
		footer = INTL_DISP_MAIN_URBAN;
		break;
	case DESK_PAGE_CLIMATE_INSIGHT:
		icon = wi_thermometer_cropped_34x32;
		iw = 34;
		ih = 32;
		src_icon = insight_32x32;
		title = INTL_DESK_TITLE_CLIMATE;
		footer = INTL_DISP_MAIN_INSIGHT;
		break;
	case DESK_PAGE_CO2:
		icon = co2_svgrepo_com_32x32;
		iw = 32;
		ih = 32;
		src_icon = insight_32x32;
		title = INTL_DESK_TITLE_CO2;
		footer = INTL_DESK_FOOTER_CO2;
		break;
	case DESK_PAGE_NOISE:
		icon = ear_hearing_34x34;
		src_icon = urban_32x32;
		title = INTL_DESK_TITLE_NOISE;
		footer = INTL_DESK_FOOTER_NOISE;
		break;
	case DESK_PAGE_PRESSURE:
		icon = pressure_32x32;
		iw = 32;
		ih = 32;
		src_icon = cfg::standalone ? insight_32x32 : nullptr;
		title = INTL_DESK_TITLE_PRESSURE;
		footer = INTL_DESK_FOOTER_PRESSURE;
		break;
	default:
		break;
	}
	const uint16_t title_bot = deskDrawTitle(content_left + 6, title_y, content_right, icon, iw, ih, title, src_icon);
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

	char a[20], b[20];

	switch (page_id) {
	case DESK_PAGE_AIR: {
		const uint8_t p10p = (!deskNoDataGeneric(values.pm10) && values.pm10 > 0.f && values.pm10 < 1.f) ? 1 : 0;
		const uint8_t p25p = (!deskNoDataGeneric(values.pm25) && values.pm25 > 0.f && values.pm25 < 1.f) ? 1 : 0;
		deskFormat(a, sizeof(a), values.pm25, p25p, false);
		deskFormat(b, sizeof(b), values.pm10, p10p, false);
		deskDrawTwoUp(left_x, right_x, col_w, mid, inner_y0, inner_y1,
		              "", "PM2.5", a, "µg/m³",
		              "", "PM10", b, "µg/m³",
		              deskBandPm(values.pm25, values.pm10), false);
		break;
	}
	case DESK_PAGE_CLIMATE:
		deskFormat(a, sizeof(a), tempCToDisplay(values.temp_outdoor), 0, true);
		deskFormat(b, sizeof(b), values.hum_outdoor, 0, false);
		deskDrawTwoUp(left_x, right_x, col_w, mid, inner_y0, inner_y1,
		              "", INTL_DISP_TEMPERATURE, a, cfgTempDisplayUnit(),
		              "", INTL_DISP_HUMIDITY, b, "%",
		              nullptr, false, true,
		              deskBandOneTemp(values.temp_outdoor),
		              deskBandOneHum(values.hum_outdoor));
		break;
	case DESK_PAGE_CLIMATE_INSIGHT:
		deskFormat(a, sizeof(a), tempCToDisplay(values.temp_indoor), 0, true);
		deskFormat(b, sizeof(b), values.hum_indoor, 0, false);
		deskDrawTwoUp(left_x, right_x, col_w, mid, inner_y0, inner_y1,
		              "", INTL_DISP_TEMPERATURE, a, cfgTempDisplayUnit(),
		              "", INTL_DISP_HUMIDITY, b, "%",
		              nullptr, false, true,
		              deskBandOneTemp(values.temp_indoor),
		              deskBandOneHum(values.hum_indoor));
		break;
	case DESK_PAGE_CO2:
		deskFormat(a, sizeof(a), values.co2, 0, false);
		deskDrawCo2(inner_x0, inner_w, inner_y0, inner_y1, a, deskBandCo2(values.co2));
		break;
	case DESK_PAGE_NOISE:
		deskFormat(a, sizeof(a), values.noise_avg, 0, false);
		deskFormat(b, sizeof(b), values.noise_max, 0, false);
		deskDrawTwoUp(left_x, right_x, col_w, mid, inner_y0, inner_y1,
		              "", "AVG", a, "dBA",
		              "", "MAX", b, "dBA",
		              deskBandNoise(values.noise_avg, values.noise_max), false);
		break;
	case DESK_PAGE_PRESSURE:
	default: {
		deskFormat(a, sizeof(a), pressureMmHgToDisplay(values.press_outdoor), 0, false);
		deskFormat(b, sizeof(b), pressureMmHgToDisplay(values.press_indoor), 0, false);
		const char *pu = cfgPressureDisplayUnit();
		deskDrawTwoUp(left_x, right_x, col_w, mid, inner_y0, inner_y1,
		              cfg::standalone ? "" : INTL_DISP_MAIN_URBAN, "", a, pu,
		              cfg::standalone ? "" : INTL_DISP_MAIN_INSIGHT, "", b, pu,
		              deskBandPress(values.press_outdoor, values.press_indoor),
		              cfg::standalone);
		break;
	}
	}
	deskDrawFooter(content_left, content_right, pos_1based, total, footer);
}

#endif
