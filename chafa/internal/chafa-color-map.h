/* SPDX-License-Identifier: LGPL-3.0-or-later */
#ifndef CHAFA_COLOR_MAP_H
#define CHAFA_COLOR_MAP_H

#include "chafa.h"
#include "internal/chafa-color.h"

/* Immutable mapping data; independent of terminal encoding and glyphs. */
typedef struct
{
    const GArray *entries;
    ChafaColor *colors;
}
ChafaColorMap;

void chafa_color_map_init (ChafaColorMap *map, const GArray *entries, ChafaColorSpace space);
void chafa_color_map_deinit (ChafaColorMap *map);
guint chafa_color_map_find_nearest (const ChafaColorMap *map, const ChafaColor *color);

#endif
