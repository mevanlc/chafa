/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* SPDX-License-Identifier: LGPL-3.0-or-later */

#ifndef __CHICLE_COLOR_H__
#define __CHICLE_COLOR_H__

#include <glib.h>
#include <chafa.h>

G_BEGIN_DECLS

/* RGB values indexed from zero. The owner keeps the name and colors alive
 * for as long as the palette is used. */
typedef struct
{
    const gchar *name;
    const guint32 *colors;
    guint n_colors;
}
ChicleColorPalette;

/* Built-in palettes live for the process lifetime; unknown names return NULL. */
const ChicleColorPalette *chicle_color_palette_find (const gchar *name);

/* Failed lookups and parses leave color_out unchanged. */
gboolean chicle_color_palette_get_color (const ChicleColorPalette *palette,
                                        guint index, guint32 *color_out);
gboolean chicle_parse_color (const gchar *str, guint32 *color_out);

/* Resolve and validate an ordered list for one output channel. The returned
 * array contains ChafaColorPaletteEntry values and is owned by the caller. */
GArray *chicle_parse_color_list (const gchar *str, ChafaCanvasMode mode,
                                gboolean foreground, const gchar *default_color,
                                GError **error);

G_END_DECLS

#endif /* __CHICLE_COLOR_H__ */
