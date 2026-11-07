/* SPDX-License-Identifier: LGPL-3.0-or-later */
#include "config.h"
#include "internal/chafa-color-map.h"

void
chafa_color_map_init (ChafaColorMap *map, const GArray *entries, ChafaColorSpace space)
{
    guint i;

    map->entries = entries;
    map->colors = NULL;
    if (!entries) return;
    map->colors = g_new (ChafaColor, entries->len);
    for (i = 0; i < entries->len; i++)
    {
        const ChafaColorPaletteEntry *entry = &g_array_index (entries, ChafaColorPaletteEntry, i);
        ChafaColor rgb;

        chafa_unpack_color (entry->rgb, &rgb);
        rgb.ch [3] = 255;
        if (space == CHAFA_COLOR_SPACE_DIN99D)
            chafa_color_rgb_to_din99d (&rgb, &map->colors [i]);
        else
            map->colors [i] = rgb;
    }
}

void
chafa_color_map_deinit (ChafaColorMap *map)
{
    g_free (map->colors);
}

guint
chafa_color_map_find_nearest (const ChafaColorMap *map, const ChafaColor *color)
{
    guint best = 0, i;
    gint best_error = G_MAXINT;

    for (i = 0; i < map->entries->len; i++)
    {
        gint error = chafa_color_diff_fast (color, &map->colors [i]);
        if (error < best_error)
        {
            best_error = error;
            best = i;
        }
    }
    return best;
}
