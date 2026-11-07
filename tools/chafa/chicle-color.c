/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* Copyright (C) 2018-2025 Hans Petter Jansson
 * SPDX-License-Identifier: LGPL-3.0-or-later */

#include "config.h"

#include <string.h>

#include "chicle-color.h"
#include "chicle-named-colors.h"

static const guint32 kitty_system_colors [16] =
{
    /* Fixed values from kitty16.txt, independent of the terminal theme. */
    0x000000, 0xcc0403, 0x19cb00, 0xcecb00,
    0x0d73cc, 0xcb1ed1, 0x0dcdcd, 0xdddddd,
    0x767676, 0xf2201f, 0x23fd00, 0xfffd00,
    0x1a8fff, 0xfd28ff, 0x14ffff, 0xffffff
};

static const guint32 chafa_system_colors [16] =
{
    /* Match the library's fixed system palette in chafa-palette.c. */
    0x000000, 0x800000, 0x007000, 0x707000,
    0x000070, 0x700070, 0x007070, 0xc0c0c0,
    0x404040, 0xff0000, 0x00ff00, 0xffff00,
    0x0000ff, 0xff00ff, 0x00ffff, 0xffffff
};

static guint32 kitty_colors [256];
static guint32 chafa_colors [256];

static const ChicleColorPalette builtin_palettes [] =
{
    { "kitty", kitty_colors, G_N_ELEMENTS (kitty_colors) },
    { "chafa", chafa_colors, G_N_ELEMENTS (chafa_colors) }
};

static guint32
xterm_extension_color (guint index)
{
    static const guint8 levels [6] = { 0, 95, 135, 175, 215, 255 };
    guint gray;

    if (index < 232)
    {
        index -= 16;
        return (levels [index / 36] << 16)
            | (levels [(index / 6) % 6] << 8)
            | levels [index % 6];
    }

    gray = 8 + 10 * (index - 232);
    return (gray << 16) | (gray << 8) | gray;
}

const ChicleColorPalette *
chicle_color_palette_find (const gchar *name)
{
    static gsize initialized;
    guint i;

    if (g_once_init_enter (&initialized))
    {
        memcpy (kitty_colors, kitty_system_colors, sizeof kitty_system_colors);
        memcpy (chafa_colors, chafa_system_colors, sizeof chafa_system_colors);

        for (i = 16; i < 256; i++)
        {
            kitty_colors [i] = xterm_extension_color (i);
            chafa_colors [i] = kitty_colors [i];
        }

        g_once_init_leave (&initialized, 1);
    }

    for (i = 0; i < G_N_ELEMENTS (builtin_palettes); i++)
    {
        if (!g_ascii_strcasecmp (name, builtin_palettes [i].name))
            return &builtin_palettes [i];
    }

    return NULL;
}

gboolean
chicle_color_palette_get_color (const ChicleColorPalette *palette,
                                guint index, guint32 *color_out)
{
    if (index >= palette->n_colors)
        return FALSE;

    *color_out = palette->colors [index];
    return TRUE;
}

gboolean
chicle_parse_color (const gchar *str, guint32 *color_out)
{
    const ChicleNamedColor *named_color;
    gchar *label;
    gchar *p;
    gsize len;
    guint32 color;

    named_color = chicle_find_color_by_name (str);
    if (named_color)
    {
        *color_out = (named_color->color [0] << 16)
            | (named_color->color [1] << 8)
            | named_color->color [2];
        return TRUE;
    }

    str += strspn (str, " \t");

    /* One to three bare digits are decimal palette indices, including leading
     * zeros. Six-digit RGB hex values remain unambiguous without a prefix. */
    len = strspn (str, "0123456789");
    if (len >= 1 && len <= 3 && str [len] == '\0')
    {
        const ChicleColorPalette *palette = chicle_color_palette_find ("kitty");
        guint64 index = g_ascii_strtoull (str, NULL, 10);

        if (index >= palette->n_colors)
            return FALSE;

        return chicle_color_palette_get_color (palette, (guint) index, color_out);
    }

    len = strspn (str, "#abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    if (len == 0)
        return FALSE;

    p = label = g_ascii_strdown (str, len);

    /* Hex triplet, optionally prefixed with # or 0x. */
    if (*p == '#') p++;
    else if (p [0] == '0' && p [1] == 'x') p += 2;

    len = strspn (p, "0123456789abcdef");
    if (len != strlen (p) || len < 6)
    {
        g_free (label);
        return FALSE;
    }

    if (len > 6) p += len - 6;

    color = (guint32) g_ascii_strtoull (p, NULL, 16);
    g_free (label);
    *color_out = color;
    return TRUE;
}
