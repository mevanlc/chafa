/* SPDX-License-Identifier: LGPL-3.0-or-later */
#include "config.h"
#include <chafa.h>
#include <string.h>
#include "chicle-color.h"
#include "internal/chafa-canvas-internal.h"

static GArray *
parse (const gchar *text, ChafaCanvasMode mode, gboolean fg, const gchar *def)
{
    GError *error = NULL;
    GArray *entries = chicle_parse_color_list (text, mode, fg, def, &error);
    g_assert_no_error (error);
    g_assert_nonnull (entries);
    return entries;
}

static void
parse_test (void)
{
    GArray *a = parse (" 001-003 , #cc0403, 007 ", CHAFA_CANVAS_MODE_INDEXED_16, TRUE, "1");
    g_assert_cmpuint (a->len, ==, 4);
    g_assert_cmpint (g_array_index (a, ChafaColorPaletteEntry, 0).output, ==, CHAFA_COLOR_OUTPUT_DEFAULT);
    g_assert_cmpuint (g_array_index (a, ChafaColorPaletteEntry, 1).index, ==, 2);
    g_assert_cmpuint (g_array_index (a, ChafaColorPaletteEntry, 3).index, ==, 7);
    g_array_unref (a);
    a = parse ("black,red,232-255", CHAFA_CANVAS_MODE_INDEXED_240, FALSE, NULL);
    g_assert_cmpuint (g_array_index (a, ChafaColorPaletteEntry, 0).index, ==, 16);
    g_assert_cmpuint (g_array_index (a, ChafaColorPaletteEntry, 1).index, ==, 196);
    g_assert_cmpuint (a->len, ==, 26);
    g_array_unref (a);
    a = parse ("000001,11223344,255", CHAFA_CANVAS_MODE_TRUECOLOR, FALSE, NULL);
    g_assert_cmphex (g_array_index (a, ChafaColorPaletteEntry, 0).rgb, ==, 1);
    g_assert_cmphex (g_array_index (a, ChafaColorPaletteEntry, 1).rgb, ==, 0x223344);
    g_array_unref (a);
}

static void
invalid_test (void)
{
    const struct { const gchar *text; ChafaCanvasMode mode; gboolean fg; const gchar *def; } cases [] =
    {
        { "", CHAFA_CANVAS_MODE_TRUECOLOR, TRUE, NULL },
        { "1,", CHAFA_CANVAS_MODE_TRUECOLOR, TRUE, NULL },
        { "1,,2", CHAFA_CANVAS_MODE_TRUECOLOR, TRUE, NULL },
        { "7-1", CHAFA_CANVAS_MODE_TRUECOLOR, TRUE, NULL },
        { "0-256", CHAFA_CANVAS_MODE_TRUECOLOR, TRUE, NULL },
        { "0001-0002", CHAFA_CANVAS_MODE_TRUECOLOR, TRUE, NULL },
        { "#000000-#ffffff", CHAFA_CANVAS_MODE_TRUECOLOR, TRUE, NULL },
        { "112233!", CHAFA_CANVAS_MODE_TRUECOLOR, TRUE, NULL },
        { "0", CHAFA_CANVAS_MODE_INDEXED_240, TRUE, NULL },
        { "8", CHAFA_CANVAS_MODE_INDEXED_8, TRUE, NULL },
        { "8", CHAFA_CANVAS_MODE_INDEXED_16_8, FALSE, NULL },
        { "red", CHAFA_CANVAS_MODE_INDEXED_16, TRUE, NULL },
        { "0", CHAFA_CANVAS_MODE_INDEXED_16, FALSE, "#123456" },
        { "16", CHAFA_CANVAS_MODE_INDEXED_240, FALSE, "0" }
    };
    guint i;
    for (i = 0; i < G_N_ELEMENTS (cases); i++)
    {
        GError *error = NULL;
        GArray *a = chicle_parse_color_list (cases [i].text, cases [i].mode, cases [i].fg, cases [i].def, &error);
        g_test_message ("Invalid: %s", cases [i].text);
        g_assert_null (a);
        g_assert_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
        g_error_free (error);
    }
}

static void
ownership_and_tie_test (void)
{
    ChafaCanvasConfig *config = chafa_canvas_config_new ();
    ChafaCanvasConfig *copy;
    ChafaColorPaletteEntry entries [] =
    {
        { 0x000000, CHAFA_COLOR_OUTPUT_RGB, 0 },
        { 0x020000, CHAFA_COLOR_OUTPUT_RGB, 0 }
    };
    ChafaCanvas *canvas;
    const guint8 pixel [] = { 1, 0, 0, 255 };
    guint n;

    chafa_canvas_config_set_fg_color_palette (config, entries, 2);
    chafa_canvas_config_set_bg_color_palette (config, entries, 2);
    copy = chafa_canvas_config_copy (config);
    entries [0].rgb = 0xffffff;
    chafa_canvas_config_set_fg_color_palette (config, NULL, 0);
    chafa_canvas_config_unref (config);
    g_assert_cmphex (chafa_canvas_config_get_fg_color_palette (copy, &n) [0].rgb, ==, 0);
    g_assert_cmpuint (n, ==, 2);
    chafa_canvas_config_set_geometry (copy, 1, 1);
    chafa_canvas_config_set_preprocessing_enabled (copy, FALSE);
    canvas = chafa_canvas_new (copy);
    chafa_canvas_config_unref (copy);
    chafa_canvas_draw_all_pixels (canvas, CHAFA_PIXEL_RGBA8_UNASSOCIATED, pixel, 1, 1, 4);
    ChafaColor tie_color = { { 1, 0, 0, 255 } };
    g_assert_cmpuint (chafa_color_map_find_nearest (&canvas->fg_color_map, &tie_color), ==, 0);
    g_assert_cmphex (canvas->cells [0].bg_color, ==, 0xff000000);
    chafa_canvas_unref (canvas);
}

static void
assert_member (const ChafaCanvas *canvas, const GArray *entries, guint32 encoded, gboolean is_default)
{
    guint i;
    if (canvas->config.canvas_mode == CHAFA_CANVAS_MODE_TRUECOLOR && !(encoded >> 24)) return;
    if (encoded == CHAFA_PALETTE_INDEX_TRANSPARENT) return;
    for (i = 0; i < entries->len; i++)
    {
        const ChafaColorPaletteEntry *entry = &g_array_index (entries, ChafaColorPaletteEntry, i);
        guint32 expected = canvas->config.canvas_mode == CHAFA_CANVAS_MODE_TRUECOLOR
            ? entry->rgb | 0xff000000 : entry->index;
        if (encoded == expected && is_default == (entry->output == CHAFA_COLOR_OUTPUT_DEFAULT)) return;
    }
    g_error ("Color %x (default=%d) escaped its palette", encoded, is_default);
}

static void
render_test (void)
{
    ChafaCanvasMode modes [] = { CHAFA_CANVAS_MODE_TRUECOLOR, CHAFA_CANVAS_MODE_INDEXED_256,
        CHAFA_CANVAS_MODE_INDEXED_240, CHAFA_CANVAS_MODE_INDEXED_16,
        CHAFA_CANVAS_MODE_INDEXED_16_8, CHAFA_CANVAS_MODE_INDEXED_8 };
    guint8 pixels [32 * 32 * 4];
    guint m, variant, i;
    for (i = 0; i < 32 * 32; i++)
    {
        pixels [i * 4] = (i * 17) % 256;
        pixels [i * 4 + 1] = (i * 31) % 256;
        pixels [i * 4 + 2] = (i * 47) % 256;
        pixels [i * 4 + 3] = i % 7 ? 255 : 0;
    }
    for (m = 0; m < G_N_ELEMENTS (modes); m++)
        for (variant = 0; variant < 16; variant++)
        {
            ChafaCanvasConfig *config = chafa_canvas_config_new ();
            ChafaCanvas *canvas;
            ChafaSymbolMap *symbols = chafa_symbol_map_new ();
            GArray *fg = parse (modes [m] == CHAFA_CANVAS_MODE_INDEXED_240 ? "196,46" : "1,2", modes [m], TRUE, NULL);
            GArray *bg = parse (modes [m] == CHAFA_CANVAS_MODE_INDEXED_240 ? "21,232" : "4,7", modes [m], FALSE, NULL);
            chafa_set_n_threads (variant & 1 ? 1 : 4);
            chafa_canvas_config_set_canvas_mode (config, modes [m]);
            chafa_canvas_config_set_color_space (config, variant & 1 ? CHAFA_COLOR_SPACE_RGB : CHAFA_COLOR_SPACE_DIN99D);
            chafa_canvas_config_set_geometry (config, 6, 4);
            chafa_canvas_config_set_work_factor (config, variant & 2 ? 0.9 : 0.1);
            chafa_canvas_config_set_preprocessing_enabled (config, FALSE);
            chafa_symbol_map_add_by_tags (symbols, CHAFA_SYMBOL_TAG_ALL);
            chafa_canvas_config_set_symbol_map (config, symbols);
            if (variant & 4) chafa_canvas_config_set_fill_symbol_map (config, symbols);
            chafa_symbol_map_unref (symbols);
            chafa_canvas_config_set_fg_color_palette (config, (const ChafaColorPaletteEntry *) fg->data, fg->len);
            if (variant & 8) chafa_canvas_config_set_fg_only_enabled (config, TRUE);
            else chafa_canvas_config_set_bg_color_palette (config, (const ChafaColorPaletteEntry *) bg->data, bg->len);
            canvas = chafa_canvas_new (config);
            chafa_canvas_config_unref (config);
            chafa_canvas_draw_all_pixels (canvas, CHAFA_PIXEL_RGBA8_UNASSOCIATED, pixels, 32, 32, 32 * 4);
            for (i = 0; i < 24; i++)
            {
                assert_member (canvas, fg, canvas->cells [i].fg_color, canvas->cells [i].fg_default);
                if (variant & 8)
                    g_assert_true (modes [m] == CHAFA_CANVAS_MODE_TRUECOLOR ? !(canvas->cells [i].bg_color >> 24) : canvas->cells [i].bg_color == CHAFA_PALETTE_INDEX_TRANSPARENT);
                else
                    assert_member (canvas, bg, canvas->cells [i].bg_color, canvas->cells [i].bg_default);
            }
            chafa_canvas_unref (canvas);
            g_array_unref (fg);
            g_array_unref (bg);
        }
}

/* Interpret SGR state at painted glyphs, rather than testing only substrings. */
static void
assert_default_states (const gchar *text, gboolean indexed, gboolean foreground_only)
{
    gint fg = -1, bg = -1;
    gboolean bold = foreground_only;
    guint seen = 0;
    const gchar *p = text;
    while (*p)
    {
        if (p [0] == '\033' && p [1] == '[')
        {
            gint values [16], n = 0, i;
            p += 2;
            while (g_ascii_isdigit (*p) && n < 16)
            {
                gchar *end;
                values [n++] = (gint) g_ascii_strtoll (p, &end, 10);
                p = end;
                if (*p != ';') break;
                p++;
            }
            g_assert_cmpint (*p, ==, 'm');
            for (i = 0; i < n; i++)
            {
                gint v = values [i];
                if (v == 0) { fg = bg = -1; bold = FALSE; }
                else if (v == 1) bold = TRUE;
                else if (v == 22) bold = FALSE;
                else if (v == 39) fg = -1;
                else if (v == 49) bg = -1;
                else if (v >= 30 && v <= 37) fg = v - 30;
                else if (v >= 40 && v <= 47) bg = v - 40;
                else if ((v == 38 || v == 48) && i + 4 < n)
                {
                    gint rgb;
                    g_assert_cmpint (values [i + 1], ==, 2);
                    rgb = (values [i + 2] << 16) | (values [i + 3] << 8) | values [i + 4];
                    if (v == 38) fg = rgb; else bg = rgb;
                    i += 4;
                }
            }
        }
        else if (*p >= 'A' && *p <= 'C')
        {
            if (*p == 'A')
            {
                g_assert_cmpint (fg, ==, indexed ? 1 : 0xf2201f);
                g_assert_cmpint (bg, ==, foreground_only ? -1 : indexed ? 4 : 0x0d73cc);
                g_assert_cmpint (bold, ==, indexed);
            }
            else
            {
                g_assert_cmpint (fg, ==, -1);
                g_assert_false (bold);
                g_assert_cmpint (bg, ==, foreground_only || *p == 'C' ? -1 : indexed ? 4 : 0x0d73cc);
            }
            seen++;
        }
        p++;
    }
    g_assert_cmpuint (seen, ==, 3);
}

static void
default_output_test (void)
{
    guint opt, mode, foreground_only;
    for (foreground_only = 0; foreground_only < 2; foreground_only++)
    for (mode = 0; mode < 2; mode++)
        for (opt = 0; opt < 2; opt++)
        {
            ChafaCanvasConfig *config = chafa_canvas_config_new ();
            ChafaCanvas *canvas;
            ChafaTermDb *db = chafa_term_db_new ();
            ChafaTermInfo *term = chafa_term_db_get_fallback_info (db);
            GString *out;
            ChafaCanvasMode cmode = mode ? CHAFA_CANVAS_MODE_INDEXED_16_8 : CHAFA_CANVAS_MODE_TRUECOLOR;
            GArray *fg = parse ("9,1", cmode, TRUE, "1");
            GArray *bg = parse ("4,7", cmode, FALSE, "7");
            guint i;

            chafa_canvas_config_set_canvas_mode (config, cmode);
            chafa_canvas_config_set_geometry (config, 3, 1);
            chafa_canvas_config_set_optimizations (config, opt ? CHAFA_OPTIMIZATION_ALL : CHAFA_OPTIMIZATION_NONE);
            chafa_canvas_config_set_fg_color_palette (config, (const ChafaColorPaletteEntry *) fg->data, fg->len);
            if (foreground_only) chafa_canvas_config_set_fg_only_enabled (config, TRUE);
            else chafa_canvas_config_set_bg_color_palette (config, (const ChafaColorPaletteEntry *) bg->data, bg->len);
            canvas = chafa_canvas_new (config);
            /* Construct a bright -> default-FG -> both-default transition. */
            for (i = 0; i < 3; i++)
            {
                canvas->cells [i].c = 'A' + i;
                canvas->cells [i].fg_color = mode ? (i ? 1 : 9) : (i ? 0xffcc0403 : 0xfff2201f);
                canvas->cells [i].bg_color = mode ? (i == 2 ? 7 : 4) : (i == 2 ? 0xffdddddd : 0xff0d73cc);
                canvas->cells [i].fg_default = i > 0;
                canvas->cells [i].bg_default = i == 2;
            }
            canvas->needs_clear = FALSE;
            out = chafa_canvas_print (canvas, term);
            assert_default_states (out->str, mode, foreground_only);
            g_assert_nonnull (strstr (out->str, "\033[39m"));
            if (foreground_only)
            {
                g_assert_null (strstr (out->str, "\033[49m"));
                g_assert_null (strstr (out->str, "\033[0m"));
            }
            else g_assert_nonnull (strstr (out->str, "\033[49m"));
            g_assert_nonnull (strchr (out->str, 'A'));
            g_assert_nonnull (strchr (out->str, 'B'));
            g_assert_nonnull (strchr (out->str, 'C'));
            g_assert_null (strstr (out->str, "\033[7m"));
            if (mode && opt) g_assert_nonnull (strstr (out->str, "\033[22m"));
            g_string_free (out, TRUE);
            chafa_canvas_unref (canvas);
            chafa_canvas_config_unref (config);
            chafa_term_info_unref (term);
            chafa_term_db_unref (db);
            g_array_unref (fg);
            g_array_unref (bg);
        }
}

static void
large_palette_test (void)
{
    GString *text = g_string_new (NULL);
    GArray *entries;
    ChafaColorMap map;
    ChafaColor target = { { 0, 1, 44, 255 } };
    guint i;
    for (i = 1; i <= 300; i++)
        g_string_append_printf (text, "%s#%06x", i == 1 ? "" : ",", i);
    entries = parse (text->str, CHAFA_CANVAS_MODE_TRUECOLOR, TRUE, NULL);
    g_assert_cmpuint (entries->len, ==, 300);
    chafa_color_map_init (&map, entries, CHAFA_COLOR_SPACE_RGB);
    g_assert_cmpuint (chafa_color_map_find_nearest (&map, &target), ==, 299);
    chafa_color_map_deinit (&map);
    g_array_unref (entries);
    g_string_free (text, TRUE);
}

static void
glyph_choice_test (void)
{
    ChafaCanvasConfig *config = chafa_canvas_config_new ();
    ChafaSymbolMap *symbols = chafa_symbol_map_new ();
    ChafaCanvas *canvas;
    ChafaColorPaletteEntry fg = { 0x0000ff, CHAFA_COLOR_OUTPUT_RGB, 0 };
    ChafaColorPaletteEntry bg = { 0xff0000, CHAFA_COLOR_OUTPUT_RGB, 0 };
    guint8 pixels [8 * 8 * 4];
    guint i;

    for (i = 0; i < 64; i++)
    {
        pixels [i * 4] = i < 32 ? 255 : 0;
        pixels [i * 4 + 1] = 0;
        pixels [i * 4 + 2] = i < 32 ? 0 : 255;
        pixels [i * 4 + 3] = 255;
    }
    chafa_symbol_map_add_by_range (symbols, 0x2580, 0x2580);
    chafa_symbol_map_add_by_range (symbols, 0x2584, 0x2584);
    chafa_canvas_config_set_symbol_map (config, symbols);
    chafa_canvas_config_set_geometry (config, 1, 1);
    chafa_canvas_config_set_work_factor (config, 0.9);
    chafa_canvas_config_set_preprocessing_enabled (config, FALSE);
    chafa_canvas_config_set_fg_color_palette (config, &fg, 1);
    chafa_canvas_config_set_bg_color_palette (config, &bg, 1);
    canvas = chafa_canvas_new (config);
    chafa_canvas_draw_all_pixels (canvas, CHAFA_PIXEL_RGBA8_UNASSOCIATED, pixels, 8, 8, 32);
    /* Blue is only legal in FG, so the lower-half block is the exact match. */
    g_assert_cmpuint (chafa_canvas_get_char_at (canvas, 0, 0), ==, 0x2584);
    chafa_canvas_unref (canvas);
    chafa_canvas_config_unref (config);
    chafa_symbol_map_unref (symbols);
}

static void
cell_setter_test (void)
{
    guint mode;
    for (mode = 0; mode < 2; mode++)
    {
        ChafaCanvasMode cmode = mode ? CHAFA_CANVAS_MODE_INDEXED_16 : CHAFA_CANVAS_MODE_TRUECOLOR;
        ChafaCanvasConfig *config = chafa_canvas_config_new ();
        ChafaCanvas *canvas;
        GArray *fg = parse ("1,4", cmode, TRUE, "1");
        GArray *bg = parse ("0", cmode, FALSE, "0");
        const guint8 pixel [] = { 204, 4, 3, 255 };
        gint rgb, bg_rgb;

        chafa_canvas_config_set_canvas_mode (config, cmode);
        chafa_canvas_config_set_geometry (config, 2, 1);
        chafa_canvas_config_set_fg_color_palette (config, (const ChafaColorPaletteEntry *) fg->data, fg->len);
        chafa_canvas_config_set_bg_color_palette (config, (const ChafaColorPaletteEntry *) bg->data, bg->len);
        canvas = chafa_canvas_new (config);
        chafa_canvas_draw_all_pixels (canvas, CHAFA_PIXEL_RGBA8_UNASSOCIATED, pixel, 1, 1, 4);
        chafa_canvas_set_colors_at (canvas, 0, 0, 0xcc0403, 0);
        chafa_canvas_get_colors_at (canvas, 0, 0, &rgb, &bg_rgb);
        g_assert_cmphex (rgb, ==, 0xcc0403);
        g_assert_cmpint (chafa_canvas_set_char_at (canvas, 0, 0, 0x4e00), ==, 2);
        g_assert_true (canvas->cells [1].fg_default);
        g_assert_true (canvas->cells [1].bg_default);
        chafa_canvas_set_colors_at (canvas, 1, 0, 0x0d73cc, 0);
        g_assert_false (canvas->cells [0].fg_default);
        g_assert_true (canvas->cells [0].bg_default);
        chafa_canvas_set_raw_colors_at (canvas, 1, 0, mode ? 1 : 0xcc0403, 0);
        g_assert_false (canvas->cells [0].fg_default);
        g_assert_false (canvas->cells [0].bg_default);
        chafa_canvas_unref (canvas);
        chafa_canvas_config_unref (config);
        g_array_unref (fg);
        g_array_unref (bg);
    }
}

int
main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);
    g_test_add_func ("/force-color/parse", parse_test);
    g_test_add_func ("/force-color/invalid", invalid_test);
    g_test_add_func ("/force-color/ownership-and-tie", ownership_and_tie_test);
    g_test_add_func ("/force-color/render", render_test);
    g_test_add_func ("/force-color/default-output", default_output_test);
    g_test_add_func ("/force-color/glyph-choice", glyph_choice_test);
    g_test_add_func ("/force-color/large-palette", large_palette_test);
    g_test_add_func ("/force-color/cell-setters", cell_setter_test);
    return g_test_run ();
}
