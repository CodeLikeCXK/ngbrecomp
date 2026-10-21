/*
 * The frame rate overlay (F10, Show FPS) lettered like the game's HUD: the numbers in the italic serif of
 * the karma count (EB Garamond ExtraBold Italic, ngb_font_tab), the words after them in grey, on a dark
 * strip with a thin rule under it as the game's own labels have. The renderer copies it over the
 * screen's corner (xboxrecomp's present_sdl.c, nv2a_fps_render), so it is opaque, and grey and white
 * only -- the same in either byte order of the swapchain.
 */
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

extern const unsigned char ngb_font_tab[];

#define BACK   0xFF121212u
#define RULE   0xFF5A5A5Au
#define NUMBER 0xFFFFFFFFu
#define WORD   0xFFA8A8A8u

static uint32_t mix(uint32_t bg, uint32_t fg, unsigned a)
{
    uint32_t out = 0xFF000000u;
    for (int sh = 0; sh < 24; sh += 8) {
        unsigned b = (bg >> sh) & 0xFF, f = (fg >> sh) & 0xFF;
        out |= ((b * (255 - a) + f * a) / 255) << sh;
    }
    return out;
}

static bool fps_rasterize(const char *text, uint32_t line_px, uint32_t *px, uint32_t maxw, uint32_t maxh,
                          uint32_t *ow, uint32_t *oh);

extern bool ngb_autosave_banner_active(void);
extern int ngb_pc_setting(int which);

/* Called at every present, for each frame's buffer; the text changes twice a second. Rasterized once and
 * copied after: drawing it every frame was 9% of the game thread's time with the counter on. */
bool nv2a_fps_render(const char *text, uint32_t line_px, uint32_t *px, uint32_t maxw, uint32_t maxh,
                     uint32_t *ow, uint32_t *oh)
{
    static char last[64];
    static uint32_t last_line, last_mw, last_mh, w, h;
    static uint32_t *cache;
    static bool ok;

    char display_text[64];
    if (ngb_autosave_banner_active()) {
        if (ngb_pc_setting(5) && text && *text)
            snprintf(display_text, sizeof(display_text), "[ AUTOSAVED ]  %s", text);
        else
            snprintf(display_text, sizeof(display_text), "[ AUTOSAVED ]");
    } else {
        snprintf(display_text, sizeof(display_text), "%s", text ? text : "");
    }

    if (!cache || strncmp(display_text, last, sizeof last) || line_px != last_line || maxw != last_mw || maxh != last_mh) {
        uint32_t *grown = realloc(cache, (size_t)maxw * maxh * 4);
        if (!grown)
            return fps_rasterize(display_text, line_px, px, maxw, maxh, ow, oh);
        cache = grown;
        snprintf(last, sizeof last, "%s", display_text);
        last_line = line_px, last_mw = maxw, last_mh = maxh;
        ok = fps_rasterize(display_text, line_px, cache, maxw, maxh, &w, &h);
    }
    if (!ok)
        return false;
    memcpy(px, cache, (size_t)w * h * 4);
    *ow = w;
    *oh = h;
    return true;
}

static bool fps_rasterize(const char *text, uint32_t line_px, uint32_t *px, uint32_t maxw, uint32_t maxh,
                          uint32_t *ow, uint32_t *oh)
{
    static stbtt_fontinfo font;
    static int ready = -1;
    static uint8_t glyph[256 * 256];
    if (ready < 0)
        ready = stbtt_InitFont(&font, ngb_font_tab, 0) != 0;
    if (!ready || line_px < 8)
        return false;

    float size = (float)line_px, scale = stbtt_ScaleForPixelHeight(&font, size);
    int asc, desc, gap;
    stbtt_GetFontVMetrics(&font, &asc, &desc, &gap);
    float pad = size * 0.45f, base = pad + asc * scale;
    uint32_t h = (uint32_t)lroundf(base - desc * scale + pad * 0.8f) + 2;

    /* the width first: each character's advance, the gaps between words a little wider */
    float width = 0;
    for (const char *s = text; *s; s++) {
        int adv, lsb;
        stbtt_GetCodepointHMetrics(&font, (unsigned char)*s, &adv, &lsb);
        width += adv * scale * (*s == ' ' ? 1.3f : 1.0f);
    }
    uint32_t w = (uint32_t)lroundf(width + pad * 2);
    if (w > maxw) w = maxw;
    if (h > maxh) h = maxh;

    for (uint32_t i = 0; i < w * h; i++)
        px[i] = BACK;
    for (uint32_t x = (uint32_t)pad / 2; x + (uint32_t)pad / 2 < w; x++)   /* the rule */
        px[(h - 2) * w + x] = RULE;

    float x = pad;
    for (const char *s = text; *s; s++) {
        int cp = (unsigned char)*s, adv, lsb, x0, y0, x1, y1;
        bool number = (cp >= '0' && cp <= '9') || cp == '.' ||
                      (cp == '-' && (s[1] == '-' || (s > text && s[-1] == '-')));
        uint32_t ink = number ? NUMBER : WORD;
        float fx = x - floorf(x);
        stbtt_GetCodepointHMetrics(&font, cp, &adv, &lsb);
        stbtt_GetCodepointBitmapBoxSubpixel(&font, cp, scale, scale, fx, 0, &x0, &y0, &x1, &y1);
        int gw = x1 - x0, gh = y1 - y0;
        if (gw > 0 && gh > 0 && gw <= 256 && gh <= 256) {
            stbtt_MakeCodepointBitmapSubpixel(&font, glyph, gw, gh, gw, scale, scale, fx, 0, cp);
            for (int yy = 0; yy < gh; yy++)
                for (int xx = 0; xx < gw; xx++) {
                    int X = (int)floorf(x) + x0 + xx, Y = (int)lroundf(base) + y0 + yy;
                    unsigned a = glyph[yy * gw + xx];
                    if (a && X >= 0 && Y >= 0 && (uint32_t)X < w && (uint32_t)Y < h)
                        px[Y * w + X] = mix(px[Y * w + X], ink, a);
                }
        }
        x += adv * scale * (cp == ' ' ? 1.3f : 1.0f);
        if (s[1])
            x += stbtt_GetCodepointKernAdvance(&font, cp, (unsigned char)s[1]) * scale;
    }
    *ow = w;
    *oh = h;
    return true;
}
