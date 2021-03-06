/* -*- Text: C; tab-width: 8; indent-tabs-text: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2012 Red Hat, Inc.
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Written by Matthias Clasen
 */

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "panel-cell-renderer-text.h"

enum {
        ACTIVATE,
        LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (PanelCellRendererText, panel_cell_renderer_text, GTK_TYPE_CELL_RENDERER_TEXT)

static gint
activate (GtkCellRenderer      *cell,
          GdkEvent             *event,
          GtkWidget            *widget,
          const gchar          *path,
          const GdkRectangle   *background_area,
          const GdkRectangle   *cell_area,
          GtkCellRendererState  flags)
{
	GdkDevice *device = gdk_event_get_device (event);

	if (device)
		gdk_device_ungrab (device, GDK_CURRENT_TIME);

        g_signal_emit (cell,  signals[ACTIVATE], 0, path);

        return TRUE;
}

static void
panel_cell_renderer_text_class_init (PanelCellRendererTextClass *class)
{
        GObjectClass *object_class = G_OBJECT_CLASS (class);
        GtkCellRendererClass *cell_renderer_class = GTK_CELL_RENDERER_CLASS (class);

        cell_renderer_class->activate = activate;

        signals[ACTIVATE] =
                g_signal_new ("activate",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (PanelCellRendererTextClass, activate),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
panel_cell_renderer_text_init (PanelCellRendererText *renderer)
{
}

GtkCellRenderer *
panel_cell_renderer_text_new (void)
{
        return g_object_new (PANEL_TYPE_CELL_RENDERER_TEXT, NULL);
}
