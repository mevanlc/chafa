/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "config.h"

#include <chafa.h>
#include "internal/chafa-palette.h"
#include "chicle-color.h"

static void
assert_color (const gchar *str, guint32 expected)
{
    guint32 color = 0xffffffff;

    g_test_message ("Color: %s", str);
    g_assert_true (chicle_parse_color (str, &color));
    g_assert_cmphex (color, ==, expected);
}

static void
system_colors_test (void)
{
    static const guint32 expected [] =
    {
        0x000000, 0xcc0403, 0x19cb00, 0xcecb00,
        0x0d73cc, 0xcb1ed1, 0x0dcdcd, 0xdddddd,
        0x767676, 0xf2201f, 0x23fd00, 0xfffd00,
        0x1a8fff, 0xfd28ff, 0x14ffff, 0xffffff
    };
    guint i;

    for (i = 0; i < G_N_ELEMENTS (expected); i++)
    {
        gchar str [32];

        g_snprintf (str, sizeof str, "%u", i);
        assert_color (str, expected [i]);
        g_snprintf (str, sizeof str, "%02u", i);
        assert_color (str, expected [i]);
        g_snprintf (str, sizeof str, "%03u", i);
        assert_color (str, expected [i]);
    }
}

static void
palette_reference_test (void)
{
    const ChicleColorPalette *kitty = chicle_color_palette_find ("kitty");
    const ChicleColorPalette *chafa = chicle_color_palette_find ("chafa");
    ChafaPalette reference;
    guint i;

    g_assert_nonnull (kitty);
    g_assert_nonnull (chafa);
    g_assert_cmpuint (kitty->n_colors, ==, 256);
    g_assert_cmpuint (chafa->n_colors, ==, 256);
    g_assert_true (chicle_color_palette_find ("KITTY") == kitty);
    g_assert_null (chicle_color_palette_find ("unknown"));

    /* Compare against the library's existing, independently tabulated palette. */
    chafa_palette_init (&reference, CHAFA_PALETTE_TYPE_FIXED_256);
    for (i = 0; i < 256; i++)
    {
        const ChafaColor *ref_color = chafa_palette_get_color (&reference, CHAFA_COLOR_SPACE_RGB, i);
        guint32 expected = (ref_color->ch [0] << 16) | (ref_color->ch [1] << 8) | ref_color->ch [2];
        guint32 actual;

        g_assert_true (chicle_color_palette_get_color (chafa, i, &actual));
        g_assert_cmphex (actual, ==, expected);

        if (i >= 16)
        {
            gchar str [32];

            g_assert_true (chicle_color_palette_get_color (kitty, i, &actual));
            g_assert_cmphex (actual, ==, expected);
            g_snprintf (str, sizeof str, "%u", i);
            assert_color (str, expected);
            g_snprintf (str, sizeof str, "%03u", i);
            assert_color (str, expected);
        }
    }
    chafa_palette_deinit (&reference);
}

static void
custom_palette_test (void)
{
    static const guint32 colors [] = { 0x123456, 0xabcdef, 0x654321 };
    const ChicleColorPalette palette = { "custom", colors, G_N_ELEMENTS (colors) };
    guint32 color = 0;

    g_assert_true (chicle_color_palette_get_color (&palette, 0, &color));
    g_assert_cmphex (color, ==, 0x123456);
    g_assert_true (chicle_color_palette_get_color (&palette, 2, &color));
    g_assert_cmphex (color, ==, 0x654321);
    g_assert_false (chicle_color_palette_get_color (&palette, 3, &color));
    g_assert_cmphex (color, ==, 0x654321);
    g_assert_false (chicle_color_palette_get_color (&palette, G_MAXUINT, &color));
}

static void
names_and_hex_test (void)
{
    assert_color ("red", 0xff0000);
    assert_color ("LIGHT SKY BLUE", 0x87cefa);
    assert_color ("abcdef", 0xabcdef);
    assert_color ("#112233", 0x112233);
    assert_color ("0x112233", 0x112233);
    assert_color ("0XABCDEF", 0xabcdef);
    assert_color ("#000001", 0x000001);
    assert_color ("#11223344", 0x223344);
    assert_color ("112233", 0x112233);
    assert_color ("000001", 0x000001);
    assert_color ("000255", 0x000255);
    assert_color ("11223344", 0x223344);
}

static void
invalid_colors_test (void)
{
    static const gchar *invalid [] =
    {
        "", "256", "999", "0256", "0001", "00001", "99999",
        "-1", "+1", "1.0", "1e2", "0x1", "#001", "rgb(1,2,3)", "no-such-color"
    };
    guint i;

    for (i = 0; i < G_N_ELEMENTS (invalid); i++)
    {
        guint32 color = 0x123456;

        g_test_message ("Invalid color: %s", invalid [i]);
        g_assert_false (chicle_parse_color (invalid [i], &color));
        g_assert_cmphex (color, ==, 0x123456);
    }
}

int
main (int argc, char *argv [])
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/tool-color/system-colors", system_colors_test);
    g_test_add_func ("/tool-color/palette-reference", palette_reference_test);
    g_test_add_func ("/tool-color/custom-palette", custom_palette_test);
    g_test_add_func ("/tool-color/names-and-hex", names_and_hex_test);
    g_test_add_func ("/tool-color/invalid-colors", invalid_colors_test);

    return g_test_run ();
}
