#ifdef ALTRUIST_INSIGHT

#include "desk_screen.h"
#include "analytics.h"
#include "display_common.h"
#include "../driver/EPD.h"
#include "../paint_driver/GUI_Paint.h"
#include "../paint_driver/fonts/fonts.h"
#include "../utils.h"
#include "../../intl.h"
#include "../../defines.h"
#include "../../config_manager/config_helpers.h"
#include <math.h>
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

static uint8_t deskGraphSeriesCount(uint8_t page_id) {
	switch (page_id) {
	case DESK_PAGE_AIR:
	case DESK_PAGE_CLIMATE:
	case DESK_PAGE_CLIMATE_INSIGHT:
		return 2;
	case DESK_PAGE_PRESSURE:
		return cfg::standalone ? 1 : 2;
	default:
		return 1;
	}
}

uint8_t deskCarouselCount() {
	const uint8_t mask = deskEffectivePagesMask();
	uint8_t n = 0;
	for (uint8_t i = 0; i < DESK_PAGE_COUNT; i++) {
		const uint8_t id = kDeskOrder[i];
		if ((mask & (1u << id)) == 0) {
			continue;
		}
		n = (uint8_t)(n + 1u + deskGraphSeriesCount(id));
	}
	return n == 0 ? 1 : n;
}

void deskCarouselDecode(uint8_t slot, uint8_t *page_id, bool *is_graph,
                        uint8_t *pos_1based, uint8_t *total, uint8_t *graph_i) {
	const uint8_t metrics = deskEnabledCount();
	const uint8_t n = deskCarouselCount();
	if (metrics == 0 || n == 0) {
		if (page_id) {
			*page_id = DESK_PAGE_CLIMATE;
		}
		if (is_graph) {
			*is_graph = false;
		}
		if (pos_1based) {
			*pos_1based = 1;
		}
		if (total) {
			*total = 1;
		}
		if (graph_i) {
			*graph_i = 0;
		}
		return;
	}
	if (slot >= n) {
		slot = 0;
	}
	uint8_t remain = slot;
	uint8_t metric_i = 0;
	const uint8_t mask = deskEffectivePagesMask();
	for (uint8_t i = 0; i < DESK_PAGE_COUNT; i++) {
		const uint8_t id = kDeskOrder[i];
		if ((mask & (1u << id)) == 0) {
			continue;
		}
		const uint8_t span = (uint8_t)(1u + deskGraphSeriesCount(id));
		if (remain < span) {
			if (page_id) {
				*page_id = id;
			}
			if (is_graph) {
				*is_graph = (remain >= 1);
			}
			if (graph_i) {
				*graph_i = (remain >= 1) ? (uint8_t)(remain - 1u) : 0;
			}
			if (pos_1based) {
				*pos_1based = (uint8_t)(metric_i + 1u);
			}
			if (total) {
				*total = metrics;
			}
			return;
		}
		remain = (uint8_t)(remain - span);
		metric_i++;
	}
	if (page_id) {
		*page_id = DESK_PAGE_CLIMATE;
	}
	if (is_graph) {
		*is_graph = false;
	}
	if (graph_i) {
		*graph_i = 0;
	}
	if (pos_1based) {
		*pos_1based = 1;
	}
	if (total) {
		*total = metrics;
	}
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
	return 0;
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

unsigned deskSlideIntervalMs(bool is_graph) {
	if (is_graph) {
		return DESK_GRAPH_INTERVAL_S * 1000UL;
	}
	return deskIntervalMs();
}

void deskNormalizeConfig() {
	if (cfg::main_layout != MAIN_LAYOUT_WALL && cfg::main_layout != MAIN_LAYOUT_DESK) {
		cfg::main_layout = MAIN_LAYOUT_WALL;
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
	cfg::desk_home = DESK_PAGE_AIR;
	for (uint8_t i = 0; i < DESK_PAGE_COUNT; i++) {
		const uint8_t id = kDeskOrder[i];
		if (cfg::desk_pages & (1u << id)) {
			cfg::desk_home = id;
			break;
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

static void deskDrawLineLegend(uint16_t x, uint16_t y, const char *solid_lab, const char *dot_lab);

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
                              const unsigned char *src_icon,
                              const char *leg_solid = nullptr, const char *leg_dot = nullptr,
                              const char *subtitle = nullptr, uint16_t right_reserve = 0) {
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
	if (right_reserve > 0 && title_max > right_reserve) {
		title_max = (uint16_t)(title_max - right_reserve);
	}
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
	uint16_t bottom_text = (uint16_t)(ty + th);
	uint16_t sub_max = (title_max > cur) ? (uint16_t)(title_max - cur) : 0;
	if (subtitle && subtitle[0]) {
		sFONT *sen = &Font16;
		const Font *sru = &font_16_cyrillic;
		const Font *sas = &font_16_ascii;
		if (sub_max > 0 && deskWidth(subtitle, sen, sru, sas) > sub_max) {
			sen = &Font12;
			sru = &font_12_cyrillic;
			sas = &font_12_ascii;
		}
		const uint16_t sy = (uint16_t)(ty + th + 1);
		deskDrawString(cur, sy, subtitle, sen, sru, sas);
		bottom_text = (uint16_t)(sy + sen->Height);
	}
	if (leg_solid && leg_solid[0] && leg_dot && leg_dot[0]) {
		const uint16_t ly = (uint16_t)(bottom_text + 1);
		deskDrawLineLegend(cur, ly, leg_solid, leg_dot);
		bottom_text = (uint16_t)(ly + Font12.Height);
	}
	return (bottom_icon > bottom_text) ? bottom_icon : bottom_text;
}

static void deskDrawFooter(uint16_t content_left, uint16_t content_right,
                           uint8_t pos_1based, uint8_t total, const char *name) {
	char pager[12];
	snprintf(pager, sizeof(pager), "%02u / %02u", (unsigned)pos_1based, (unsigned)total);
	const uint16_t y = (DISPLAY_HEIGHT > 22) ? (uint16_t)(DISPLAY_HEIGHT - 20) : 0;
	deskDrawString((uint16_t)(content_left + 6), y, pager, &Font16, &font_16_cyrillic, &font_16_ascii);
	const uint16_t nw = deskWidth(name, &Font16, &font_16_cyrillic, &font_16_ascii);
	uint16_t nx = content_right > nw + 6 ? (uint16_t)(content_right - nw - 4) : content_left;
	deskDrawString(nx, y, name, &Font16, &font_16_cyrillic, &font_16_ascii);
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

static void deskFitLabel(const char *s, uint16_t max_w, sFONT **en, const Font **ru,
                         const Font **ascii) {
	static const struct {
		sFONT *en;
		const Font *ru;
		const Font *ascii;
	} cand[] = {
		{ &Font24, &font_24_cyrillic, &font_24_ascii },
		{ &Font20, &font_20_cyrillic, &font_20_ascii },
		{ &Font16, &font_16_cyrillic, &font_16_ascii },
	};
	for (unsigned i = 0; i < 3; i++) {
		*en = cand[i].en;
		*ru = cand[i].ru;
		*ascii = cand[i].ascii;
		if (!s || s[0] == '\0' || max_w == 0) {
			return;
		}
		if (deskWidth(s, *en, *ru, *ascii) <= max_w) {
			return;
		}
	}
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
		h = (uint16_t)(h + Font24.Height + 8);
	}
	if (unit) {
		h = (uint16_t)(h + 6 + Font24.Height);
	}
	(void)unit_lg;
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
		sFONT *c_en = &Font24;
		const Font *c_ru = &font_24_cyrillic;
		const Font *c_ascii = &font_24_ascii;
		if (!solo) {
			deskFitLabel(cL, w, &c_en, &c_ru, &c_ascii);
			deskDrawCentered(left_x, w, y, cL, c_en, c_ru, c_ascii);
		}
		deskFitLabel(cR, w, &c_en, &c_ru, &c_ascii);
		deskDrawCentered(x1, w, y, cR, c_en, c_ru, c_ascii);
		y = (uint16_t)(y + Font24.Height + 8);
	}
	if (!solo) {
		deskDrawHugeWithSide(left_x, w, y, vL, "", m.font, m.scale, true);
	}
	deskDrawHugeWithSide(x1, w, y, vR, "", m.font, m.scale, true);
	uint16_t band_bot = (uint16_t)(y + ink_h);
	if (unit) {
		sFONT *uen = &Font24;
		const Font *uru = &font_24_cyrillic;
		const Font *uascii = &font_24_ascii;
		const uint16_t uy = (uint16_t)(y + ink_h + 6);
		if (!solo) {
			deskFitLabel(uL, w, &uen, &uru, &uascii);
			deskDrawCentered(left_x, w, uy, uL, uen, uru, uascii);
		}
		deskFitLabel(uR, w, &uen, &uru, &uascii);
		deskDrawCentered(x1, w, uy, uR, uen, uru, uascii);
		band_bot = (uint16_t)(uy + Font24.Height);
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
	const uint16_t ppm_w = deskWidth(ppm, &Font24, &font_24_cyrillic, &font_24_ascii);
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
	if (ink_h > Font24.Height) {
		ppm_y = (uint16_t)(ink_y + (ink_h - Font24.Height) / 2);
	}
	deskDrawString((uint16_t)(gx + huge.w + 8), ppm_y, ppm, &Font24, &font_24_cyrillic, &font_24_ascii);

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

static constexpr uint8_t kDeskBarAvg = 0;
static constexpr uint8_t kDeskBarMax = 1;
static constexpr uint8_t kDeskBarBand = 2;

static bool deskThrOn(float thr) {
	return !isnan(thr);
}

static float deskBarV(const analytics_last_hours_t &h, uint8_t i, uint8_t pick,
                      float fill_hi, float fill_lo) {
	if (!h.has[i]) {
		return 0.f;
	}
	const float mx = (h.maxv[i] > h.avg[i]) ? h.maxv[i] : h.avg[i];
	const float mn = h.minv[i];
	if (pick == kDeskBarBand) {
		if (deskThrOn(fill_lo) && mn <= fill_lo) {
			return mn;
		}
		if (deskThrOn(fill_hi) && mx >= fill_hi) {
			return mx;
		}
		return h.avg[i];
	}
	if (pick == kDeskBarMax) {
		return mx;
	}
	return h.avg[i];
}

static void deskOverlayLive(analytics_last_hours_t &h, float live, bool ok) {
	if (!ok || h.n == 0) {
		return;
	}
	const uint8_t i = (uint8_t)(h.n - 1u);
	if (!h.has[i]) {
		h.has[i] = true;
		h.avg[i] = live;
		h.maxv[i] = live;
		h.minv[i] = live;
		h.found++;
		return;
	}
	if (live > h.maxv[i]) {
		h.maxv[i] = live;
	}
	if (live < h.minv[i]) {
		h.minv[i] = live;
	}
}

static void deskHoursApply(analytics_last_hours_t &h, float (*fn)(float)) {
	if (!fn) {
		return;
	}
	for (uint8_t i = 0; i < h.n; i++) {
		if (!h.has[i]) {
			continue;
		}
		h.avg[i] = fn(h.avg[i]);
		h.maxv[i] = fn(h.maxv[i]);
		h.minv[i] = fn(h.minv[i]);
	}
}

static bool deskHoursEnvelope(const analytics_last_hours_t &h, float *vmin, float *vmax) {
	bool any = false;
	float lo = 0.f, hi = 0.f;
	for (uint8_t i = 0; i < h.n; i++) {
		if (!h.has[i]) {
			continue;
		}
		const float mx = (h.maxv[i] > h.avg[i]) ? h.maxv[i] : h.avg[i];
		const float mn = (h.minv[i] < h.avg[i]) ? h.minv[i] : h.avg[i];
		if (!any) {
			lo = mn;
			hi = mx;
			any = true;
		} else {
			if (mn < lo) {
				lo = mn;
			}
			if (mx > hi) {
				hi = mx;
			}
		}
	}
	if (vmin) {
		*vmin = lo;
	}
	if (vmax) {
		*vmax = hi;
	}
	return any;
}

static bool deskHoursLineEnvelope(const analytics_last_hours_t &h, float *vmin, float *vmax) {
	bool any = false;
	float lo = 0.f, hi = 0.f;
	for (uint8_t i = 0; i < h.n; i++) {
		if (!h.has[i]) {
			continue;
		}
		const float v = h.avg[i];
		if (!any) {
			lo = v;
			hi = v;
			any = true;
		} else {
			if (v < lo) {
				lo = v;
			}
			if (v > hi) {
				hi = v;
			}
		}
	}
	if (vmin) {
		*vmin = lo;
	}
	if (vmax) {
		*vmax = hi;
	}
	return any;
}

static void deskUnionHours(const analytics_last_hours_t &a, const analytics_last_hours_t &b,
                           analytics_last_hours_t &out) {
	out = a.found ? a : b;
	if (!a.found || !b.found) {
		return;
	}
	out.n = a.n;
	out.found = 0;
	for (uint8_t i = 0; i < out.n; i++) {
		const bool ha = a.has[i];
		const bool hb = i < b.n && b.has[i];
		out.has[i] = ha || hb;
		if (!out.has[i]) {
			continue;
		}
		out.found++;
		out.hour[i] = ha ? a.hour[i] : b.hour[i];
		float mn = 0.f, mx = 0.f;
		bool any = false;
		if (ha) {
			mn = (a.minv[i] < a.avg[i]) ? a.minv[i] : a.avg[i];
			mx = (a.maxv[i] > a.avg[i]) ? a.maxv[i] : a.avg[i];
			any = true;
		}
		if (hb) {
			const float bmn = (b.minv[i] < b.avg[i]) ? b.minv[i] : b.avg[i];
			const float bmx = (b.maxv[i] > b.avg[i]) ? b.maxv[i] : b.avg[i];
			if (!any || bmn < mn) {
				mn = bmn;
			}
			if (!any || bmx > mx) {
				mx = bmx;
			}
		}
		out.minv[i] = mn;
		out.maxv[i] = mx;
		out.avg[i] = (mn + mx) * 0.5f;
	}
}

static bool deskFindPeakTrough(const analytics_last_hours_t &h, uint8_t *peak_i,
                               uint8_t *trough_i) {
	bool any = false;
	uint8_t pi = 0, ti = 0;
	float p = 0.f, t = 0.f;
	for (uint8_t i = 0; i < h.n; i++) {
		if (!h.has[i]) {
			continue;
		}
		const float mx = (h.maxv[i] > h.avg[i]) ? h.maxv[i] : h.avg[i];
		const float mn = (h.minv[i] < h.avg[i]) ? h.minv[i] : h.avg[i];
		if (!any || mx > p) {
			p = mx;
			pi = i;
		}
		if (!any || mn < t) {
			t = mn;
			ti = i;
		}
		any = true;
	}
	if (peak_i) {
		*peak_i = pi;
	}
	if (trough_i) {
		*trough_i = ti;
	}
	return any;
}

static bool deskHoursStats(const analytics_last_hours_t &h, uint8_t pick, float fill_hi,
                           float fill_lo, float *first, float *last, float *peak, uint8_t *peak_h) {
	bool have_first = false;
	bool any = false;
	float f = 0.f, l = 0.f, p = 0.f;
	uint8_t ph = 0;
	for (uint8_t i = 0; i < h.n; i++) {
		if (!h.has[i]) {
			continue;
		}
		const float v = deskBarV(h, i, pick, fill_hi, fill_lo);
		if (!have_first) {
			f = v;
			have_first = true;
		}
		l = v;
		if (!any || v > p) {
			p = v;
			ph = h.hour[i];
		}
		any = true;
	}
	if (first) {
		*first = f;
	}
	if (last) {
		*last = l;
	}
	if (peak) {
		*peak = p;
	}
	if (peak_h) {
		uint8_t pi = 0, ti = 0;
		if (deskFindPeakTrough(h, &pi, &ti) && h.has[pi]) {
			ph = h.hour[pi];
		}
		*peak_h = ph;
	}
	return any;
}

static uint8_t deskHoursAgo(const analytics_last_hours_t &h) {
	int first = -1, last = -1;
	for (uint8_t i = 0; i < h.n; i++) {
		if (!h.has[i]) {
			continue;
		}
		if (first < 0) {
			first = (int)i;
		}
		last = (int)i;
	}
	if (first < 0 || last <= first) {
		return 0;
	}
	return (uint8_t)(last - first);
}

static void deskScalePad(float *ymin, float *ymax, float min_span, float pad_frac) {
	float lo = *ymin;
	float hi = *ymax;
	if (hi < lo) {
		const float t = lo;
		lo = hi;
		hi = t;
	}
	float span = hi - lo;
	if (span < min_span) {
		const float mid = (hi + lo) * 0.5f;
		lo = mid - min_span * 0.5f;
		hi = mid + min_span * 0.5f;
		span = min_span;
	}
	const float pad = span * pad_frac;
	*ymin = lo - pad;
	*ymax = hi + pad;
}

static void deskDrawInsight(uint16_t x, uint16_t w, uint16_t y, const char *text) {
	if (!text || text[0] == '\0' || w < 8) {
		return;
	}
	sFONT *en = &Font16;
	const Font *ru = &font_16_cyrillic;
	const Font *ascii = &font_16_ascii;
	const uint16_t tw = deskWidth(text, en, ru, ascii);
	if (tw <= w) {
		deskDrawString(deskCenterX(x, w, tw), y, text, en, ru, ascii);
		return;
	}
	const char *split = nullptr;
	for (const char *p = text; *p; p++) {
		if (*p != ' ') {
			continue;
		}
		char buf[96];
		size_t n = (size_t)(p - text);
		if (n >= sizeof(buf)) {
			n = sizeof(buf) - 1;
		}
		memcpy(buf, text, n);
		buf[n] = 0;
		if (deskWidth(buf, en, ru, ascii) <= w) {
			split = p;
		} else {
			break;
		}
	}
	if (!split) {
		deskDrawString(x, y, text, en, ru, ascii);
		return;
	}
	char line1[96];
	size_t n1 = (size_t)(split - text);
	if (n1 >= sizeof(line1)) {
		n1 = sizeof(line1) - 1;
	}
	memcpy(line1, text, n1);
	line1[n1] = 0;
	const char *line2 = split + 1;
	const uint16_t w1 = deskWidth(line1, en, ru, ascii);
	const uint16_t w2 = deskWidth(line2, en, ru, ascii);
	deskDrawString(deskCenterX(x, w, w1), y, line1, en, ru, ascii);
	deskDrawString(deskCenterX(x, w, w2), (uint16_t)(y + Font16.Height + 2), line2, en, ru, ascii);
}


static void deskFillInsight(char *out, size_t n, uint8_t page_id,
                            const analytics_last_hours_t &a, uint8_t a_pick,
                            float a_hi, float a_lo,
                            const analytics_last_hours_t *b, uint8_t b_pick,
                            float b_hi, float b_lo, uint8_t graph_i) {
	if (!out || n == 0) {
		return;
	}
	out[0] = 0;
	float first = 0, last = 0, peak_line = 0;
	uint8_t peak_h = 0;
	if (!deskHoursStats(a, kDeskBarAvg, NAN, NAN, &first, &last, &peak_line, &peak_h)) {
		out[0] = 0;
		return;
	}
	float peak = 0;
	(void)deskHoursStats(a, kDeskBarMax, NAN, NAN, nullptr, nullptr, &peak, &peak_h);
	(void)a_pick;
	(void)a_hi;
	(void)a_lo;
	const float d = last - first;
	const uint8_t ago = deskHoursAgo(a);
	const bool have_span = (a.found >= 2 && ago >= 1);
	switch (page_id) {
	case DESK_PAGE_AIR:
		if (!have_span) {
			break;
		}
		if (d <= -5.f) {
			snprintf(out, n, INTL_DESK_GRAPH_CLEANER, (unsigned)ago);
		} else if (d >= 5.f) {
			snprintf(out, n, INTL_DESK_GRAPH_DIRTIER, (unsigned)ago);
		} else {
			snprintf(out, n, INTL_DESK_GRAPH_PEAK, (unsigned)peak_h);
		}
		break;
	case DESK_PAGE_CO2:
		if (!have_span) {
			break;
		}
		if (peak >= 2000.f) {
			snprintf(out, n, "%s", INTL_DESK_GRAPH_CO2_HIGH);
		} else if (last < 1000.f && peak < 1000.f) {
			snprintf(out, n, "%s", INTL_DESK_GRAPH_STABLE_CO2);
		} else if (d >= 150.f) {
			snprintf(out, n, INTL_DESK_GRAPH_ROSE, (unsigned)lroundf(d), (unsigned)ago);
		} else if (d <= -150.f) {
			snprintf(out, n, INTL_DESK_GRAPH_DROPPED, (unsigned)lroundf(-d), (unsigned)ago);
		} else {
			snprintf(out, n, INTL_DESK_GRAPH_PEAK, (unsigned)peak_h);
		}
		break;
	case DESK_PAGE_NOISE:
		if (peak < 50.f) {
			snprintf(out, n, "%s", INTL_DESK_GRAPH_QUIET);
		} else if (have_span) {
			snprintf(out, n, INTL_DESK_GRAPH_LOUD, (unsigned)peak_h);
		}
		break;
	case DESK_PAGE_CLIMATE:
	case DESK_PAGE_CLIMATE_INSIGHT: {
		(void)b;
		(void)b_pick;
		(void)b_hi;
		(void)b_lo;
		if (graph_i >= 1) {
			if (last < 40.f) {
				snprintf(out, n, "%s", INTL_DESK_GRAPH_HUM_LOW);
			} else if (last >= 70.f) {
				snprintf(out, n, "%s", INTL_DESK_GRAPH_HUM_HIGH);
			} else if (have_span) {
				snprintf(out, n, INTL_DESK_GRAPH_PEAK, (unsigned)peak_h);
			}
			break;
		}
		if (page_id == DESK_PAGE_CLIMATE_INSIGHT && !cfg::standalone) {
			analytics_last_hours_t urb{};
			analyticsReadLastHours(analytics_hour_series_t::TempUrban, urb);
			deskHoursApply(urb, tempCToDisplay);
			float ufirst, ulast, upeak;
			uint8_t uh;
			if (deskHoursStats(urb, kDeskBarAvg, NAN, NAN, &ufirst, &ulast, &upeak, &uh) &&
			    urb.found >= 1) {
				const int delta = (int)lroundf(last - ulast);
				if (delta >= 2) {
					snprintf(out, n, INTL_DESK_GRAPH_IN_WARMER, delta);
					break;
				}
				if (delta <= -2) {
					snprintf(out, n, INTL_DESK_GRAPH_IN_COOLER, -delta);
					break;
				}
			}
		}
		if (have_span && d >= 1.5f) {
			snprintf(out, n, INTL_DESK_GRAPH_WARMER, (unsigned)ago);
		} else if (have_span && d <= -1.5f) {
			snprintf(out, n, INTL_DESK_GRAPH_COOLER, (unsigned)ago);
		} else if (have_span) {
			snprintf(out, n, INTL_DESK_GRAPH_PEAK, (unsigned)peak_h);
		}
		break;
	}
	case DESK_PAGE_PRESSURE:
	default:
		if (!have_span) {
			break;
		}
		if (d <= -1.5f) {
			snprintf(out, n, "%s", INTL_DESK_GRAPH_PRESS_FALL);
		} else if (d >= 1.5f) {
			snprintf(out, n, "%s", INTL_DESK_GRAPH_PRESS_RISE);
		} else {
			snprintf(out, n, "%s", INTL_DESK_GRAPH_PRESS_STEADY);
		}
		break;
	}
}

static void deskPrepScaleEnv(const analytics_last_hours_t &h, float floor_min, float floor_max,
                             float min_span, bool pin_floor, float fill_hi, float fill_lo,
                             float *ymin, float *ymax, float *peak) {
	float lo = 0.f, hi = 0.f;
	if (!deskHoursLineEnvelope(h, &lo, &hi)) {
		*ymin = floor_min;
		*ymax = floor_max;
		if (peak) {
			*peak = 0.f;
		}
		return;
	}
	float ext_lo = lo, ext_hi = hi;
	if (deskHoursEnvelope(h, &ext_lo, &ext_hi)) {
		const float span = (hi > lo) ? (hi - lo) : min_span;
		const float gate = (span * 1.8f > min_span * 2.f) ? span * 1.8f : min_span * 2.f;
		if (ext_hi <= hi + gate && ext_hi > hi) {
			hi = ext_hi;
		}
		if (ext_lo >= lo - gate && ext_lo < lo) {
			lo = ext_lo;
		}
	}
	if (peak) {
		*peak = ext_hi;
	}
	if (pin_floor && lo > floor_min) {
		lo = floor_min;
	}
	auto pull_thr = [&](float t) {
		if (!deskThrOn(t)) {
			return;
		}
		if (t > hi && t - hi <= min_span) {
			hi = t;
		}
		if (t < lo && lo - t <= min_span) {
			lo = t;
		}
	};
	pull_thr(fill_hi);
	pull_thr(fill_lo);
	const float data_lo = lo;
	deskScalePad(&lo, &hi, min_span, 0.12f);
	if (data_lo >= 0.f && lo < 0.f) {
		lo = 0.f;
	}
	*ymin = lo;
	*ymax = hi;
}

static void deskFmtPeak(char *out, size_t n, float v, const char *unit) {
	if (!out || n == 0) {
		return;
	}
	if (unit && unit[0] == '%') {
		snprintf(out, n, "%.0f%%", v);
	} else if (unit && unit[0]) {
		snprintf(out, n, "%.0f%s", v, unit);
	} else {
		snprintf(out, n, "%.0f", v);
	}
}

static void deskDrawHeroMax(uint16_t right, uint16_t y, const char *hero, const char *unit) {
	if (!hero || hero[0] == '\0') {
		return;
	}
	const uint16_t max_w =
	    deskWidth(INTL_DESK_GRAPH_MAX, &Font12, &font_12_cyrillic, &font_12_ascii);
	const uint16_t num_w = deskWidth(hero, &Font24, &font_24_cyrillic, &font_24_ascii);
	const uint16_t unit_w = (unit && unit[0])
	                            ? deskWidth(unit, &Font16, &font_16_cyrillic, &font_16_ascii)
	                            : 0;
	const uint16_t value_w = (uint16_t)(num_w + (unit_w ? unit_w + 6 : 0));
	const uint16_t block_w = (max_w > value_w) ? max_w : value_w;
	if (right < block_w) {
		return;
	}
	deskDrawString((uint16_t)(right - max_w), y, INTL_DESK_GRAPH_MAX, &Font12, &font_12_cyrillic,
	               &font_12_ascii);
	const uint16_t ny = (uint16_t)(y + Font12.Height + 6);
	const uint16_t value_x = (uint16_t)(right - value_w);
	deskDrawString(value_x, ny, hero, &Font24, &font_24_cyrillic, &font_24_ascii);
	if (unit && unit[0]) {
		uint16_t uy = ny;
		if (Font24.Height > Font16.Height) {
			uy = (uint16_t)(ny + (Font24.Height - Font16.Height) / 2);
		}
		deskDrawString((uint16_t)(value_x + num_w + 6), uy, unit, &Font16, &font_16_cyrillic,
		               &font_16_ascii);
	}
}

struct desk_line_plot_t {
	uint16_t left, right, top, bottom;
	float ymin, ymax;
};

static uint16_t deskLineY(const desk_line_plot_t &p, float v) {
	const float span = p.ymax - p.ymin;
	if (span <= 0.01f) {
		return p.bottom;
	}
	float t = (v - p.ymin) / span;
	if (t < 0.f) {
		t = 0.f;
	}
	if (t > 1.f) {
		t = 1.f;
	}
	return (uint16_t)(p.bottom - (uint16_t)lroundf(t * (float)(p.bottom - p.top)));
}

static int deskLineX(const desk_line_plot_t &p, uint8_t n, uint8_t i) {
	if (n <= 1) {
		return (int)((p.left + p.right) / 2);
	}
	if (i >= n) {
		i = (uint8_t)(n - 1u);
	}
	return (int)p.left + (int)i * ((int)p.right - (int)p.left) / (int)(n - 1u);
}

static void deskDrawPeakTag(const desk_line_plot_t &p, int cx, uint16_t cy, const char *lab) {
	if (!lab || lab[0] == '\0') {
		return;
	}
	sFONT *en = &Font12;
	const Font *ru = &font_12_cyrillic;
	const Font *ascii = &font_12_ascii;
	const uint16_t lw = deskWidth(lab, en, ru, ascii);
	int lx = cx - (int)lw / 2;
	int ly = (int)cy - (int)Font12.Height - 6;
	if (lx < (int)p.left) {
		lx = (int)p.left;
	}
	if (lx + (int)lw > (int)p.right) {
		lx = (int)p.right - (int)lw;
	}
	if (ly < 0) {
		ly = 0;
	}
	Paint_DrawRectangle((UWORD)lx, (UWORD)ly, (UWORD)(lx + (int)lw),
	                    (UWORD)(ly + Font12.Height), WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
	deskDrawString((uint16_t)lx, (uint16_t)ly, lab, en, ru, ascii);
}

static void deskDrawLineLegend(uint16_t x, uint16_t y, const char *solid_lab, const char *dot_lab) {
	if (!solid_lab || !solid_lab[0] || !dot_lab || !dot_lab[0]) {
		return;
	}
	sFONT *en = &Font12;
	const Font *ru = &font_12_cyrillic;
	const Font *ascii = &font_12_ascii;
	const uint16_t aw = deskWidth(solid_lab, en, ru, ascii);
	const uint16_t bw = deskWidth(dot_lab, en, ru, ascii);
	const int seg = 14;
	int cx = (int)x;
	const uint16_t mid = (uint16_t)(y + Font12.Height / 2);
	Paint_DrawLine((UWORD)cx, mid, (UWORD)(cx + seg), mid, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
	cx += seg + 4;
	deskDrawString((uint16_t)cx, y, solid_lab, en, ru, ascii);
	cx += (int)aw + 12;
	Paint_DrawLine((UWORD)cx, mid, (UWORD)(cx + seg), mid, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
	cx += seg + 4;
	deskDrawString((uint16_t)cx, y, dot_lab, en, ru, ascii);
	(void)bw;
}

static bool deskOpenLinePlot(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                             float ymin, float ymax, float fill_hi, float fill_lo,
                             bool hours, const analytics_last_hours_t *hour_src,
                             desk_line_plot_t *out) {
	if (!out || w < 36 || h < 40) {
		return false;
	}
	uint16_t plot_y = y;
	const uint16_t label_h = hours ? (uint16_t)(Font12.Height + 2) : 0;
	const uint16_t y_lab_h = (uint16_t)(Font12.Height + 2);
	if (h <= (uint16_t)(plot_y - y) + label_h + y_lab_h + 16) {
		return false;
	}
	const uint16_t plot_bottom = (uint16_t)(y + h - label_h);
	const uint16_t plot_top = (uint16_t)(plot_y + y_lab_h);
	char hi_lab[12];
	char lo_lab[12];
	snprintf(hi_lab, sizeof(hi_lab), "%.0f", ymax);
	snprintf(lo_lab, sizeof(lo_lab), "%.0f", ymin);
	const uint16_t hi_w = deskWidth(hi_lab, &Font12, &font_12_cyrillic, &font_12_ascii);
	const uint16_t lo_w = deskWidth(lo_lab, &Font12, &font_12_cyrillic, &font_12_ascii);
	const uint16_t axis_w = (uint16_t)((hi_w > lo_w ? hi_w : lo_w) + 4);
	const uint16_t plot_left = (uint16_t)(x + axis_w);
	const uint16_t plot_right = (uint16_t)(x + w);
	if (plot_right <= plot_left + 16 || plot_bottom <= plot_top + 12) {
		return false;
	}
	deskDrawString((uint16_t)(plot_left - 2 - hi_w), plot_y, hi_lab, &Font12,
	               &font_12_cyrillic, &font_12_ascii);
	deskDrawString((uint16_t)(plot_left - 2 - lo_w),
	               (uint16_t)(plot_bottom - Font12.Height), lo_lab, &Font12,
	               &font_12_cyrillic, &font_12_ascii);
	Paint_DrawLine(plot_left, plot_top, plot_left, plot_bottom, BLACK, DOT_PIXEL_1X1,
	               LINE_STYLE_SOLID);
	Paint_DrawLine(plot_left, plot_bottom, plot_right, plot_bottom, BLACK, DOT_PIXEL_1X1,
	               LINE_STYLE_SOLID);
	out->left = plot_left;
	out->right = plot_right;
	out->top = plot_top;
	out->bottom = plot_bottom;
	out->ymin = ymin;
	out->ymax = ymax;
	(void)fill_hi;
	(void)fill_lo;
	if (hours && hour_src && hour_src->n > 0) {
		sFONT *hen = &Font12;
		const Font *hru = &font_12_cyrillic;
		const Font *hasc = &font_12_ascii;
		const int slot = ((int)plot_right - (int)plot_left) / (int)hour_src->n;
		if (slot < 22) {
			hen = &Font8;
			hru = &font_8_cyrillic;
			hasc = &font_8_ascii;
		}
		const uint16_t hy = (uint16_t)(plot_bottom + 2);
		for (uint8_t i = 0; i < hour_src->n; i++) {
			char hh[8];
			snprintf(hh, sizeof(hh), "%02u", (unsigned)hour_src->hour[i]);
			const uint16_t hw = deskWidth(hh, hen, hru, hasc);
			int hx = deskLineX(*out, hour_src->n, i) - (int)hw / 2;
			if (hx < (int)plot_left) {
				hx = (int)plot_left;
			}
			if (hx + (int)hw > (int)plot_right) {
				hx = (int)plot_right - (int)hw;
			}
			deskDrawString((uint16_t)hx, hy, hh, hen, hru, hasc);
		}
	}
	return true;
}

static void deskFillArea(const desk_line_plot_t &p, const analytics_last_hours_t &series) {
	int prev_x = 0;
	int prev_y = 0;
	bool have_prev = false;
	for (uint8_t i = 0; i < series.n; i++) {
		if (!series.has[i]) {
			continue;
		}
		const int cx = deskLineX(p, series.n, i);
		const int cy = (int)deskLineY(p, series.avg[i]);
		if (have_prev && cx > prev_x) {
			for (int x = prev_x; x <= cx; x++) {
				const int y = prev_y + (int)((int32_t)(cy - prev_y) * (x - prev_x) / (cx - prev_x));
				int y0 = y + 3;
				if (y0 < (int)p.top) {
					y0 = (int)p.top;
				}
				for (int py = y0; py < (int)p.bottom; py++) {
					if (((x + py) & 1) == 0) {
						Paint_SetPixel((UWORD)x, (UWORD)py, BLACK);
					}
				}
			}
		}
		prev_x = cx;
		prev_y = cy;
		have_prev = true;
	}
}

static void deskStrokeSeries(const desk_line_plot_t &p, const analytics_last_hours_t &series,
                             LINE_STYLE style, DOT_PIXEL width, const char *unit,
                             bool mark_peak, bool mark_trough, bool annotate, bool fill = true,
                             bool hollow = false) {
	if (series.n == 0 || series.found == 0) {
		return;
	}
	(void)unit;
	if (fill) {
		deskFillArea(p, series);
	}
	uint8_t peak_i = 0, trough_i = 0;
	const bool have_ext = deskFindPeakTrough(series, &peak_i, &trough_i);
	int prev_x = 0;
	uint16_t prev_y = 0;
	bool have_prev = false;
	for (uint8_t i = 0; i < series.n; i++) {
		if (!series.has[i]) {
			continue;
		}
		const int cx = deskLineX(p, series.n, i);
		const uint16_t cy = deskLineY(p, series.avg[i]);
		if (have_prev) {
			Paint_DrawLine((UWORD)prev_x, prev_y, (UWORD)cx, cy, BLACK, width, style);
		}
		prev_x = cx;
		prev_y = cy;
		have_prev = true;
	}
	for (uint8_t i = 0; i < series.n; i++) {
		if (!series.has[i]) {
			continue;
		}
		const int cx = deskLineX(p, series.n, i);
		const uint16_t cy = deskLineY(p, series.avg[i]);
		if (hollow) {
			Paint_DrawCircle((UWORD)cx, cy, 3, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
			Paint_DrawCircle((UWORD)cx, cy, 3, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
		} else {
			Paint_DrawCircle((UWORD)cx, cy, 3, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
			Paint_DrawCircle((UWORD)cx, cy, 2, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
		}
		const bool is_peak = have_ext && mark_peak && i == peak_i;
		if (annotate && !is_peak) {
			char lab[12];
			deskFmtPeak(lab, sizeof(lab), series.avg[i], nullptr);
			deskDrawPeakTag(p, cx, cy, lab);
		}
	}
	if (!have_ext || !series.has[peak_i]) {
		return;
	}
	if (mark_peak) {
		const float avg = series.avg[peak_i];
		const float ext = (series.maxv[peak_i] > avg) ? series.maxv[peak_i] : avg;
		const int cx = deskLineX(p, series.n, peak_i);
		const uint16_t y_avg = deskLineY(p, avg);
		const uint16_t y_ext = deskLineY(p, ext);
		if ((int)y_avg - (int)y_ext >= 2) {
			Paint_DrawLine((UWORD)cx, y_avg, (UWORD)cx, y_ext, BLACK, DOT_PIXEL_1X1,
			               LINE_STYLE_SOLID);
		}
		Paint_DrawCircle((UWORD)cx, y_ext, 4, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
		Paint_DrawCircle((UWORD)cx, y_ext, 3, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
		char lab[16];
		deskFmtPeak(lab, sizeof(lab), ext, nullptr);
		deskDrawPeakTag(p, cx, y_ext, lab);
	}
	if (mark_trough && trough_i != peak_i && series.has[trough_i]) {
		const float avg = series.avg[trough_i];
		const float ext = (series.minv[trough_i] < avg) ? series.minv[trough_i] : avg;
		const int cx = deskLineX(p, series.n, trough_i);
		const uint16_t y_avg = deskLineY(p, avg);
		const uint16_t y_ext = deskLineY(p, ext);
		if ((int)y_ext - (int)y_avg >= 8) {
			Paint_DrawLine((UWORD)cx, y_avg, (UWORD)cx, y_ext, BLACK, DOT_PIXEL_1X1,
			               LINE_STYLE_SOLID);
			Paint_DrawCircle((UWORD)cx, y_ext, 2, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
		}
	}
}

static float deskSeriesPeak(const analytics_last_hours_t &h) {
	uint8_t peak_i = 0, trough_i = 0;
	if (!deskFindPeakTrough(h, &peak_i, &trough_i) || !h.has[peak_i]) {
		return NAN;
	}
	return (h.maxv[peak_i] > h.avg[peak_i]) ? h.maxv[peak_i] : h.avg[peak_i];
}

static void deskDrawHourLine(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                             const analytics_last_hours_t &series, float ymin, float ymax,
                             float fill_hi, float fill_lo, const char *unit, bool hours,
                             const analytics_last_hours_t *overlay = nullptr,
                             bool overlay_is_max = false) {
	const bool dual = overlay && overlay->found && series.found;
	const float p1 = deskSeriesPeak(series);
	const float p2 = (overlay && overlay->found) ? deskSeriesPeak(*overlay) : NAN;
	desk_line_plot_t plot{};
	if (!deskOpenLinePlot(x, y, w, h, ymin, ymax, fill_hi, fill_lo, hours, &series, &plot)) {
		return;
	}
	if (dual) {
		const bool overlay_hi = !isnan(p2) && (isnan(p1) || p2 >= p1);
		deskStrokeSeries(plot, series, LINE_STYLE_SOLID, DOT_PIXEL_2X2, unit, !overlay_hi, false,
		                 true, true, false);
		analytics_last_hours_t mx = *overlay;
		if (overlay_is_max) {
			for (uint8_t i = 0; i < mx.n; i++) {
				if (mx.has[i] && mx.maxv[i] > mx.avg[i]) {
					mx.avg[i] = mx.maxv[i];
				}
			}
		}
		deskStrokeSeries(plot, mx, LINE_STYLE_DOTTED, DOT_PIXEL_1X1, unit, overlay_hi, false, true,
		                 false, true);
	} else {
		deskStrokeSeries(plot, series, LINE_STYLE_SOLID, DOT_PIXEL_2X2, unit, true, true, true);
	}
}

static const char *deskGraphMapType(uint8_t page_id, uint8_t graph_i) {
	switch (page_id) {
	case DESK_PAGE_AIR:
		return graph_i >= 1 ? "pm10" : "pm25";
	case DESK_PAGE_CLIMATE:
	case DESK_PAGE_CLIMATE_INSIGHT:
		return graph_i >= 1 ? "humidity" : "temperature";
	case DESK_PAGE_CO2:
		return "co2";
	case DESK_PAGE_NOISE:
		return "noise";
	default:
		return "pressure";
	}
}

static bool deskGraphMapIsUrban(uint8_t page_id, uint8_t graph_i) {
	if (page_id == DESK_PAGE_PRESSURE) {
		return !cfg::standalone && graph_i == 0;
	}
	return page_id == DESK_PAGE_AIR || page_id == DESK_PAGE_CLIMATE ||
	       page_id == DESK_PAGE_NOISE;
}

static const char *deskGraphFooter(uint8_t page_id, uint8_t graph_i) {
	switch (page_id) {
	case DESK_PAGE_AIR:
		return graph_i >= 1 ? "PM10" : "PM2.5";
	case DESK_PAGE_CLIMATE:
	case DESK_PAGE_CLIMATE_INSIGHT:
		return graph_i >= 1 ? INTL_DISP_HUMIDITY : INTL_DISP_TEMPERATURE;
	case DESK_PAGE_CO2:
		return INTL_DESK_FOOTER_CO2;
	case DESK_PAGE_NOISE:
		return INTL_DESK_FOOTER_NOISE;
	case DESK_PAGE_PRESSURE:
		if (!cfg::standalone) {
			return graph_i == 0 ? INTL_DISP_MAIN_URBAN : INTL_DISP_MAIN_INSIGHT;
		}
		return INTL_DESK_FOOTER_PRESSURE;
	default:
		return INTL_DESK_GRAPH_HOURS;
	}
}

static const char *deskGraphSubtitle(uint8_t page_id, uint8_t graph_i) {
	switch (page_id) {
	case DESK_PAGE_AIR:
		return graph_i >= 1 ? "PM10" : "PM2.5";
	case DESK_PAGE_CLIMATE:
	case DESK_PAGE_CLIMATE_INSIGHT:
		return graph_i >= 1 ? INTL_DISP_HUMIDITY : INTL_DISP_TEMPERATURE;
	case DESK_PAGE_PRESSURE:
		if (!cfg::standalone) {
			return graph_i == 0 ? INTL_DISP_MAIN_URBAN : INTL_DISP_MAIN_INSIGHT;
		}
		return nullptr;
	default:
		return nullptr;
	}
}

static void deskDrawGraphPage(const main_screen_values_t &values, uint8_t page_id,
                             uint8_t graph_i, uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1,
                             uint16_t title_y, const String &insight_ss58, const String &urban_ss58) {
	analytics_last_hours_t s{};
	analytics_last_hours_t s2{};
	const analytics_last_hours_t *overlay = nullptr;
	bool overlay_is_max = false;
	uint8_t pick = kDeskBarMax;
	float floor_min = 0.f, floor_max = 1.f, min_span = 1.f;
	float fill_hi = NAN, fill_lo = NAN;
	const char *unit = "";
	float live = 0.f;
	bool live_ok = false;
	bool apply_temp = false;
	bool apply_press = false;
	bool pin = true;

	switch (page_id) {
	case DESK_PAGE_AIR:
		if (graph_i >= 1) {
			analyticsReadLastHours(analytics_hour_series_t::Pm10, s);
			live = values.pm10;
			live_ok = !deskNoDataGeneric(values.pm10);
			fill_lo = 45.f;
			fill_hi = 100.f;
		} else {
			analyticsReadLastHours(analytics_hour_series_t::Pm25, s);
			live = values.pm25;
			live_ok = !deskNoDataGeneric(values.pm25);
			fill_lo = 15.f;
			fill_hi = 45.f;
		}
		pin = false;
		floor_min = 0.f;
		floor_max = 50.f;
		min_span = 8.f;
		unit = "µg/m³";
		break;
	case DESK_PAGE_CLIMATE:
	case DESK_PAGE_CLIMATE_INSIGHT: {
		const bool urban = (page_id == DESK_PAGE_CLIMATE);
		pick = kDeskBarBand;
		if (graph_i >= 1) {
			analyticsReadLastHours(urban ? analytics_hour_series_t::HumUrban
			                             : analytics_hour_series_t::HumIndoor, s);
			live = urban ? values.hum_outdoor : values.hum_indoor;
			live_ok = urban ? !deskNoDataGeneric(values.hum_outdoor)
			                : !deskNoDataGeneric(values.hum_indoor);
			pin = false;
			floor_min = 20.f;
			floor_max = 80.f;
			min_span = 20.f;
			fill_lo = 40.f;
			fill_hi = 70.f;
			unit = "%";
		} else {
			analyticsReadLastHours(urban ? analytics_hour_series_t::TempUrban
			                             : analytics_hour_series_t::TempIndoor, s);
			live = urban ? values.temp_outdoor : values.temp_indoor;
			live_ok = urban ? !deskNoDataTemp(values.temp_outdoor)
			                : !deskNoDataTemp(values.temp_indoor);
			apply_temp = true;
			pin = false;
			floor_min = tempCToDisplay(10.f);
			floor_max = tempCToDisplay(30.f);
			min_span = 8.f;
			fill_lo = tempCToDisplay(10.f);
			fill_hi = tempCToDisplay(27.f);
			unit = cfgTempDisplayUnit();
		}
		break;
	}
	case DESK_PAGE_CO2:
		analyticsReadLastHours(analytics_hour_series_t::Co2, s);
		live = values.co2;
		live_ok = !deskNoDataGeneric(values.co2);
		floor_min = 400.f;
		floor_max = 1600.f;
		min_span = 400.f;
		fill_hi = 1000.f;
		unit = "ppm";
		break;
	case DESK_PAGE_NOISE:
		analyticsReadLastHours(analytics_hour_series_t::NoiseAvg, s);
		analyticsReadLastHours(analytics_hour_series_t::Noise, s2);
		deskOverlayLive(s, values.noise_avg, !deskNoDataGeneric(values.noise_avg));
		deskOverlayLive(s2, values.noise_max, !deskNoDataGeneric(values.noise_max));
		live_ok = false;
		if (s.found == 0 && s2.found > 0) {
			s = s2;
			s2 = analytics_last_hours_t{};
		}
		if (s.found > 0 && s2.found > 0) {
			overlay = &s2;
			overlay_is_max = true;
		}
		floor_min = 30.f;
		floor_max = 70.f;
		min_span = 20.f;
		fill_hi = 50.f;
		unit = "dBA";
		break;
	case DESK_PAGE_PRESSURE:
	default: {
		const bool urban_p = !cfg::standalone && graph_i == 0;
		if (urban_p) {
			analyticsReadLastHours(analytics_hour_series_t::PressUrban, s);
			live = values.press_outdoor;
			live_ok = !deskNoDataGeneric(values.press_outdoor);
		} else {
			analyticsReadLastHours(analytics_hour_series_t::PressIndoor, s);
			live = values.press_indoor;
			live_ok = !deskNoDataGeneric(values.press_indoor);
		}
		apply_press = true;
		pick = kDeskBarBand;
		pin = false;
		fill_lo = pressureMmHgToDisplay(740.f);
		fill_hi = pressureMmHgToDisplay(780.f);
		floor_min = fill_lo;
		floor_max = fill_hi;
		min_span = pressureMmHgToDisplay(8.f) - pressureMmHgToDisplay(0.f);
		unit = cfgPressureDisplayUnit();
		break;
	}
	}

	deskOverlayLive(s, live, live_ok);
	if (apply_temp) {
		deskHoursApply(s, tempCToDisplay);
	}
	if (apply_press) {
		deskHoursApply(s, pressureMmHgToDisplay);
	}

	int qr_side = 0;
	int qr_x = 0;
	int qr_y = 0;
	const bool urban_sensor = deskGraphMapIsUrban(page_id, graph_i);
	const char *ss58 = nullptr;
	if (urban_sensor) {
		ss58 = urban_ss58.length() > 0 ? urban_ss58.c_str()
		                               : (insight_ss58.length() > 0 ? insight_ss58.c_str() : nullptr);
	} else {
		ss58 = insight_ss58.length() > 0 ? insight_ss58.c_str()
		                                 : (urban_ss58.length() > 0 ? urban_ss58.c_str() : nullptr);
	}
	const String map_url = buildSensorsSocialMapUrl(ss58, deskGraphMapType(page_id, graph_i));
	uint16_t plot_x1 = x1;
	if (map_url.length() > 8 && y1 > y0 + 48) {
		const int max_h = (int)(y1 - y0);
		int max_w = 96;
		if (max_w > max_h) {
			max_w = max_h;
		}
		qr_side = analyticsPrepareMapQr(map_url.c_str(), max_w, max_h);
		if (qr_side > 0 && x1 > x0 + (uint16_t)qr_side + 24) {
			plot_x1 = (uint16_t)(x1 - qr_side - 8);
			qr_x = (int)x1 - qr_side;
			qr_y = (int)y1 - qr_side;
			if (qr_y < (int)y0) {
				qr_y = (int)y0;
			}
		} else {
			qr_side = 0;
		}
	}
	auto draw_qr = [&]() {
		if (qr_side > 0) {
			analyticsDrawPreparedMapQr(qr_x, qr_y);
		}
	};

	const uint16_t w = (plot_x1 > x0) ? (uint16_t)(plot_x1 - x0) : 0;
	const uint16_t insight_h = (uint16_t)(Font16.Height * 2 + 8);
	const uint16_t plot_bot = (y1 > y0 + insight_h + 8) ? (uint16_t)(y1 - insight_h) : y1;
	char tip[96];
	tip[0] = 0;

	if (s.found < 2) {
		deskDrawInsight(x0, w, (uint16_t)(y0 + 40), INTL_DESK_GRAPH_COLLECTING);
		draw_qr();
		return;
	}

	float ymin, ymax, peak;
	if (overlay && overlay->found) {
		analytics_last_hours_t both{};
		deskUnionHours(s, *overlay, both);
		deskPrepScaleEnv(both, floor_min, floor_max, min_span, pin, fill_hi, fill_lo, &ymin, &ymax,
		                 &peak);
	} else {
		deskPrepScaleEnv(s, floor_min, floor_max, min_span, pin, fill_hi, fill_lo, &ymin, &ymax,
		                 &peak);
	}
	(void)peak;
	if (page_id == DESK_PAGE_CLIMATE || page_id == DESK_PAGE_CLIMATE_INSIGHT) {
		if (graph_i >= 1) {
			if (ymin < 0.f) {
				ymin = 0.f;
			}
			if (ymax > 100.f) {
				ymax = 100.f;
			}
		}
	}
	{
		char hero[16];
		hero[0] = 0;
		float show = deskSeriesPeak(s);
		if (overlay && overlay->found) {
			const float p2 = deskSeriesPeak(*overlay);
			if (!isnan(p2) && (isnan(show) || p2 > show)) {
				show = p2;
			}
		}
		if (!isnan(show)) {
			deskFmtPeak(hero, sizeof(hero), show, nullptr);
		}
		uint16_t icon_left =
		    (x1 + 6 > kDeskSrcDraw) ? (uint16_t)(x1 + 6 - kDeskSrcDraw) : x0;
		const uint16_t hero_right = (icon_left > 4) ? (uint16_t)(icon_left - 4) : x0;
		deskDrawHeroMax(hero_right, (uint16_t)(title_y + 4), hero, unit);
	}
	deskDrawHourLine(x0, y0, w, (uint16_t)(plot_bot > y0 ? plot_bot - y0 : 0), s, ymin, ymax,
	                 fill_hi, fill_lo, unit, true, overlay, overlay_is_max);
	deskFillInsight(tip, sizeof(tip), page_id, (page_id == DESK_PAGE_NOISE && overlay) ? *overlay : s,
	                pick, fill_hi, fill_lo, nullptr, kDeskBarAvg, NAN, NAN, graph_i);
	deskDrawInsight(x0, w, (uint16_t)(plot_bot + 4), tip);
	draw_qr();
}

void drawDeskMainBody(const main_screen_values_t &values,
                      uint16_t body_top, uint16_t content_left, uint16_t content_right,
                      uint8_t page_id, uint8_t pos_1based, uint8_t total,
                      bool is_graph, uint8_t graph_i, const String &insight_ss58,
                      const String &urban_ss58) {
	const uint16_t title_y = (uint16_t)(body_top + 2);
	const uint16_t footer_y = (DISPLAY_HEIGHT > 22) ? (uint16_t)(DISPLAY_HEIGHT - 20) : DISPLAY_HEIGHT;
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
		if (cfg::standalone) {
			src_icon = insight_32x32;
		} else if (is_graph) {
			src_icon = (graph_i == 0) ? urban_32x32 : insight_32x32;
		} else {
			src_icon = nullptr;
		}
		title = INTL_DESK_TITLE_PRESSURE;
		footer = INTL_DESK_FOOTER_PRESSURE;
		break;
	default:
		break;
	}
	if (is_graph) {
		footer = deskGraphFooter(page_id, graph_i);
	}
	const char *title_leg_s = nullptr;
	const char *title_leg_d = nullptr;
	const char *title_sub = nullptr;
	if (is_graph) {
		if (page_id == DESK_PAGE_NOISE) {
			title_leg_s = "AVG";
			title_leg_d = "MAX";
		} else {
			title_sub = deskGraphSubtitle(page_id, graph_i);
		}
	}
	uint16_t title_bot = deskDrawTitle(content_left + 6, title_y, content_right, icon, iw, ih,
	                                  title, src_icon, title_leg_s, title_leg_d, title_sub,
	                                  is_graph ? 108u : 0u);
	if (is_graph) {
		const uint16_t hero_bot =
		    (uint16_t)(title_y + 4 + Font12.Height + 6 + Font24.Height);
		if (hero_bot > title_bot) {
			title_bot = hero_bot;
		}
	}
	const uint16_t inner_x0 = (uint16_t)(content_left + 6);
	const uint16_t inner_x1 = (uint16_t)(content_right - 6);
	const uint16_t inner_y0 = (uint16_t)(title_bot + 10);
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

	if (is_graph) {
		deskDrawGraphPage(values, page_id, graph_i, inner_x0, inner_x1, inner_y0, inner_y1,
		                  title_y, insight_ss58,
		                  urban_ss58.length() > 0 ? urban_ss58 : values.urban_robonomics_address);
		deskDrawFooter(content_left, content_right, pos_1based, total, footer);
		return;
	}

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
