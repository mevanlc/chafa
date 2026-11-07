/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* Copyright (C) 2018-2025 Hans Petter Jansson
 * SPDX-License-Identifier: LGPL-3.0-or-later */

#include "config.h"

#include <string.h>
#include <stdlib.h>

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

static gint
parse_palette_index (const gchar *str)
{
    gsize len = strspn (str, "0123456789");
    if (len == 0 || len > 3 || str [len]) return -1;
    return (gint) g_ascii_strtoull (str, NULL, 10);
}

static gboolean
resolve_color (const gchar *str, ChafaCanvasMode mode, gboolean foreground,
               ChafaColorPaletteEntry *entry)
{
    const ChicleColorPalette *palette = chicle_color_palette_find ("kitty");
    const ChicleNamedColor *named = chicle_find_color_by_name (str);
    gint index = parse_palette_index (str);
    guint first = 0, last = 255, i;

    /* Unlike the legacy single-color parser, list items must be consumed in
     * full. Keep its long-hex behavior, but never discard trailing junk. */
    if (!named && strspn (str, "#abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") != strlen (str))
        return FALSE;
    if (!chicle_parse_color (str, &entry->rgb)) return FALSE;
    entry->index = 0;
    entry->output = CHAFA_COLOR_OUTPUT_RGB;
    if (mode == CHAFA_CANVAS_MODE_TRUECOLOR) return TRUE;

    switch (mode)
    {
        case CHAFA_CANVAS_MODE_INDEXED_8: last = 7; break;
        case CHAFA_CANVAS_MODE_INDEXED_16_8: last = foreground ? 15 : 7; break;
        case CHAFA_CANVAS_MODE_INDEXED_16: last = 15; break;
        case CHAFA_CANVAS_MODE_INDEXED_240: first = 16; break;
        case CHAFA_CANVAS_MODE_INDEXED_256: break;
        default: return FALSE;
    }
    if (index >= 0)
    {
        if ((guint) index < first || (guint) index > last) return FALSE;
        entry->index = (guint) index;
    }
    else
    {
        for (i = first; i <= last; i++)
            if (palette->colors [i] == entry->rgb) break;
        if (i > last) return FALSE;
        entry->index = i;
    }
    entry->output = CHAFA_COLOR_OUTPUT_INDEXED;
    return TRUE;
}

static void
append_color (GArray *entries, ChafaColorPaletteEntry entry)
{
    guint i;
    for (i = 0; i < entries->len; i++)
    {
        ChafaColorPaletteEntry *old = &g_array_index (entries, ChafaColorPaletteEntry, i);
        if (old->rgb != entry.rgb) continue;
        if (entry.output == CHAFA_COLOR_OUTPUT_DEFAULT) *old = entry;
        return;
    }
    g_array_append_val (entries, entry);
}

GArray *
chicle_parse_color_list (const gchar *str, ChafaCanvasMode mode,
                         gboolean foreground, const gchar *default_color, GError **error)
{
    GArray *entries = g_array_new (FALSE, FALSE, sizeof (ChafaColorPaletteEntry));
    gchar **items = g_strsplit (str, ",", -1);
    const gchar *bad = str;
    guint i;

    for (i = 0; items [i]; i++)
    {
        gchar *item = g_strstrip (items [i]);
        gchar *dash = strchr (item, '-');
        ChafaColorPaletteEntry entry;

        bad = item;
        if (dash)
        {
            gchar *left = g_strndup (item, dash - item);
            gint first = parse_palette_index (g_strstrip (left));
            gint last = parse_palette_index (g_strstrip (dash + 1));
            gint n;

            g_free (left);
            if (first < 0 || last < first || last > 255) goto fail;
            for (n = first; n <= last; n++)
            {
                gchar number [4];
                g_snprintf (number, sizeof number, "%d", n);
                if (!resolve_color (number, mode, foreground, &entry)) goto fail;
                append_color (entries, entry);
            }
        }
        else
        {
            if (!resolve_color (item, mode, foreground, &entry)) goto fail;
            append_color (entries, entry);
        }
    }
    if (entries->len == 0) goto fail;
    if (default_color)
    {
        ChafaColorPaletteEntry entry;
        gchar *trimmed = g_strdup (default_color);
        gboolean valid = resolve_color (g_strstrip (trimmed), mode, foreground, &entry);
        g_free (trimmed);
        bad = default_color;
        if (!valid) goto fail;
        entry.output = CHAFA_COLOR_OUTPUT_DEFAULT;
        append_color (entries, entry);
    }
    g_strfreev (items);
    return entries;

fail:
    {
        const gchar *mode_name = "unsupported";
        switch (mode)
        {
            case CHAFA_CANVAS_MODE_TRUECOLOR: mode_name = "full"; break;
            case CHAFA_CANVAS_MODE_INDEXED_256: mode_name = "256"; break;
            case CHAFA_CANVAS_MODE_INDEXED_240: mode_name = "240"; break;
            case CHAFA_CANVAS_MODE_INDEXED_16: mode_name = "16"; break;
            case CHAFA_CANVAS_MODE_INDEXED_16_8: mode_name = "16/8"; break;
            case CHAFA_CANVAS_MODE_INDEXED_8: mode_name = "8"; break;
            default: break;
        }
        g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                     "Invalid or unavailable color/range '%s' for --force-color-%s in effective color mode %s.",
                     bad, foreground ? "fg" : "bg", mode_name);
    }
    g_strfreev (items);
    g_array_unref (entries);
    return NULL;
}
